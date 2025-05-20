//; =============================================================================
//; RaistlinBars - SID Music Visualizer with Spectrum Analyzer Effect
//; (c) 2018, Raistlin of Genesis*Project
//; Revised and optimized version with improved smoothing algorithm
//; ADSR Extension added to implement complete envelope visualization
//; =============================================================================

* = PlayerADDR

    jmp MUSICPLAYER_Initialize   //; Entry point for the player

//; =============================================================================
//; RESOURCE FILES
//; =============================================================================
.var spritesine_bin = LoadBinary("SpriteSine.bin")       //; Sine wave data for sprite movement
.var freqtable_bin = LoadBinary("FreqTable.bin")         //; Lookup tables for frequency mapping
.var charset_map = LoadBinary("CharSet.map")             //; Character set for visualization
.var watersprites_map = LoadBinary("WaterSprites.map")   //; Sprites for water reflection effect

//; =============================================================================
//; DISPLAY CONFIGURATION
//; =============================================================================
.var NUM_FREQS_ON_SCREEN = 40                //; Number of frequency bars shown
.var FREQ_BARS_PADDING = 1                   //; Extra padding bytes on each side for smoothing

//; Song information display settings
.var SONG_TITLE_COLOR = $01                  //; Color for song title
.var SONG_TITLE_SCREEN_LINE = 0              //; Screen line for song title
.var SONG_TITLE_LENGTH = min(SIDName.size(), 40) //; Truncate to screen width
.var SONG_TITLE_PADDING = (40 - SONG_TITLE_LENGTH) / 2 //; Centering padding

//; Artist information display settings
.var ARTIST_NAME_COLOR = $0f                 //; Color for artist name
.var ARTIST_NAME_SCREEN_LINE = 3             //; Screen line for artist name
.var ARTIST_NAME_LENGTH = min(SIDAuthor.size(), 40) //; Truncate to screen width
.var ARTIST_NAME_PADDING = (40 - ARTIST_NAME_LENGTH) / 2 //; Centering padding

//; Visualization layout
.var TOP_SPECTROMETER_HEIGHT = 15            //; Height of main spectrometer in chars
.var TOP_SPECTROMETER_PIXELHEIGHT = (TOP_SPECTROMETER_HEIGHT * 8)
.var BOTTOM_SPECTROMETER_HEIGHT = 3          //; Height of reflection effect in chars
.var SPECTROMETER_START_LINE = 25 - (TOP_SPECTROMETER_HEIGHT + BOTTOM_SPECTROMETER_HEIGHT)

//; =============================================================================
//; MEMORY CONFIGURATION
//; =============================================================================
.var BASE_BANK = 3                           //; Memory bank (3=c000-ffff)
.var BASE_BANK_ADDRESS = (BASE_BANK * $4000) //; Start address of the bank
.var SCREEN_BANK = 14                        //; Screen memory location (Bank+[3800,3bff])
.var CHAR_BANK_FONT = 6                      //; Character set location (Bank+[3000,37ff])
.var SCREEN_ADDRESS = (BASE_BANK_ADDRESS + (SCREEN_BANK * $400)) //; Screen memory address
.var SPRITE_POINTERS = (SCREEN_ADDRESS + $3F8) //; Sprite pointer table in screen memory
.var CHARSET_ADDRESS = (BASE_BANK_ADDRESS + (CHAR_BANK_FONT * $800)) //; Character set address
.var SPRITE_INDEX_START = $80                //; First sprite index
.var WATERSPRITES_ADDRESS = (BASE_BANK_ADDRESS + (SPRITE_INDEX_START * $40)) //; Water sprites

//; =============================================================================
//; LOCAL DATA SECTION
//; =============================================================================
StartLocalData:

    //; Constant for VIC register initialization skipping
    .var SKIP_VALUE = $e1

    //; VIC-II register initialization values
    .var NUM_VIC_INIT_VALUES = EndVICInitValues - StartVICInitValues

    StartVICInitValues:
        .byte $10                                        //; D000: Sprite0X
        .byte $e5                                        //; D001: Sprite0Y
        .byte $40                                        //; D002: Sprite1X
        .byte $e5                                        //; D003: Sprite1Y
        .byte $70                                        //; D004: Sprite2X
        .byte $e5                                        //; D005: Sprite2Y
        .byte $a0                                        //; D006: Sprite3X
        .byte $e5                                        //; D007: Sprite3Y
        .byte $d0                                        //; D008: Sprite4X
        .byte $e5                                        //; D009: Sprite4Y
        .byte $00                                        //; D00A: Sprite5X
        .byte $e5                                        //; D00B: Sprite5Y
        .byte $30                                        //; D00C: Sprite6X
        .byte $e5                                        //; D00D: Sprite6Y
        .byte $00                                        //; D00E: Sprite7X
        .byte $00                                        //; D00F: Sprite7Y
        .byte $60                                        //; D010: SpriteXMSB
        .byte SKIP_VALUE                                 //; D011: Screen control register 1
        .byte SKIP_VALUE                                 //; D012: Raster line
        .byte SKIP_VALUE                                 //; D013: LightPenX
        .byte SKIP_VALUE                                 //; D014: LightPenY
        .byte $7f                                        //; D015: SpriteEnable (all enabled)
        .byte $08                                        //; D016: Screen control register 2
        .byte $00                                        //; D017: SpriteDoubleHeight
        .byte ((SCREEN_BANK * 16) + (CHAR_BANK_FONT * 2)) //; D018: Memory setup
        .byte SKIP_VALUE                                 //; D019: Interrupt status
        .byte SKIP_VALUE                                 //; D01A: Interrupt control
        .byte $00                                        //; D01B: SpriteDrawPriority
        .byte $00                                        //; D01C: SpriteMulticolourMode
        .byte $7f                                        //; D01D: SpriteDoubleWidth (all enabled)
        .byte $00                                        //; D01E: SpriteSpriteCollision
        .byte $00                                        //; D01F: SpriteBackgroundCollision
        .byte $00                                        //; D020: BorderColour
        .byte $00                                        //; D021: ScreenColour
        .byte $00                                        //; D022: MultiColour0
        .byte $00                                        //; D023: MultiColour1
        .byte $00                                        //; D024: ExtraBackgroundColour2
        .byte $00                                        //; D025: SpriteExtraColour0
        .byte $00                                        //; D026: SpriteExtraColour1
        .byte $00                                        //; D027: Sprite0Colour
        .byte $00                                        //; D028: Sprite1Colour
        .byte $00                                        //; D029: Sprite2Colour
        .byte $00                                        //; D02A: Sprite3Colour
        .byte $00                                        //; D02B: Sprite4Colour
        .byte $00                                        //; D02C: Sprite5Colour
        .byte $00                                        //; D02D: Sprite6Colour
        .byte $00                                        //; D02E: Sprite7Colour
    EndVICInitValues:

    //; Song metadata
    SongData_Title:           .text SIDName.substring(0, min(SIDName.size(), 40))
    SongData_Artist:          .text SIDAuthor.substring(0, min(SIDAuthor.size(), 40))

    //; Visualization variables
    UpdateVisualizerSignal:   .byte $00      //; Flag to trigger visualization update
    FrameCounter:             .byte $00      //; Counter for current frame (0-255)
    Frame_256Counter:         .byte $00      //; Counter for 256-frame cycles

    //; Cache for previous frame data to optimize rendering
    PrevFrame_BarHeights:     .fill NUM_FREQS_ON_SCREEN, 255  //; Previous bar heights
    PrevFrame_BarColors:      .fill NUM_FREQS_ON_SCREEN, 255  //; Previous bar colors

    .align 128
  
    SoundBarSine:             .fill 128, (TOP_SPECTROMETER_PIXELHEIGHT - 1.0) * sin(toRadians(i*90/128)) //; Sine wave data for sound bar animation

    .align 256

    Div3Table:                .fill 256, i / 3

//; New ADSR Extension - Attack and Decay tables
//; -------------------------------------------------------------------------
//; ADSR State constants
.var ADSR_ATTACK = 0
.var ADSR_DECAY = 1
.var ADSR_SUSTAIN = 2
.var ADSR_RELEASE = 3

//;  -------------------------------------------------------------------------
//; ADSR Conversion Tables
//; These tables convert the SID's ADSR values to visual representations
//; 
//; Attack Conversion Tables - converts SID attack values (0-15) to visualization rates
//; Lower attack values (0-7) produce fast attacks, higher values (8-15) produce slower attacks
//; This matches how the SID chip's envelope generator works
//; 
//; Decay Conversion Tables - converts SID decay values (0-15) to visualization rates
//; Lower decay values (0-7) produce fast decays, higher values (8-15) produce slower decays
//; 
//; Sustain: Converts sustain value (0-15) to visible bar height (35-125)
//;  - Formula: (sustain / 16) * 6 + 35
//;  - Higher sustain values in SID = taller bars in visualization
//;
//; Release: Converts release value (0-15) to bar decay rate
//;  - Formula: mod(release, 16) * 96 + 768
//;  - Stored as 16-bit value (hi/lo bytes)
//;  - Higher release values = faster visual decay of bars

    AttackConversionHi:       .fill 256, >(((15 - mod(i, 16)) + 8 ) * TOP_SPECTROMETER_PIXELHEIGHT / 2)
    AttackConversionLo:       .fill 256, <(((15 - mod(i, 16)) + 8 ) * TOP_SPECTROMETER_PIXELHEIGHT / 2)

    DecayConversionHi:        .fill 256, >(((15 - mod(i / 16, 16)) + 8) * TOP_SPECTROMETER_PIXELHEIGHT / 2)
    DecayConversionLo:        .fill 256, <(((15 - mod(i / 16, 16)) + 8) * TOP_SPECTROMETER_PIXELHEIGHT / 2)

/*    ReleaseConversionHi:       .fill 256, >(((15 - mod(i, 16)) + 8 ) * TOP_SPECTROMETER_PIXELHEIGHT)
    ReleaseConversionLo:       .fill 256, <(((15 - mod(i, 16)) + 8 ) * TOP_SPECTROMETER_PIXELHEIGHT)

    SustainConversion:        .fill 256, (mod(i / 16, 16) * (TOP_SPECTROMETER_PIXELHEIGHT - 15) / 15) + 10*/

    SustainConversion:        .fill 256, floor(i / 16) * TOP_SPECTROMETER_HEIGHT / 3 + TOP_SPECTROMETER_HEIGHT * 2 + 5
    ReleaseConversionHi:      .fill 256, >((15 - mod(i, 16)) * TOP_SPECTROMETER_HEIGHT * 8 + TOP_SPECTROMETER_HEIGHT * 60)
    ReleaseConversionLo:      .fill 256, <((15 - mod(i, 16)) * TOP_SPECTROMETER_HEIGHT * 8 + TOP_SPECTROMETER_HEIGHT * 60)


//;  -------------------------------------------------------------------------

//; ADSR state tracking
ChannelADSRState:         .fill 3, ADSR_RELEASE  //; Track current ADSR state for each channel
ChannelTargetHeights:     .fill 3, 0             //; Target heights for sustain level
ChannelAttackHi:          .fill 3, 0             //; Attack rates per channel (high byte)
ChannelAttackLo:          .fill 3, 0             //; Attack rates per channel (low byte)
ChannelDecayHi:           .fill 3, 0             //; Decay rates per channel (high byte)
ChannelDecayLo:           .fill 3, 0             //; Decay rates per channel (low byte)

//; Color tables for different ADSR phases (optional enhancement)
ADSRPhaseColors:          .byte $0e, $0a, $07, $0b  //; Colors for Attack, Decay, Sustain, Release
//; -------------------------------------------------------------------------

    //; Frequency conversion tables
    LoFreqToLookupTable:      .fill 256, i / 4
    HiFreqToLookupTable:      .fill 4, i * 64

    //; Sine wave data for sprite movement
    SpriteSine:               .fill spritesine_bin.getSize(), spritesine_bin.get(i)
    
    //; Frequency lookup tables
    FreqTable:                .fill freqtable_bin.getSize(), freqtable_bin.get(i)

    //; Analyzer data buffers with padding bytes for smoothing
    ChannelToFreqMap:               .fill NUM_FREQS_ON_SCREEN, 0                        //; Maps screen positions to SID channels
    RawBarHeightsHi:                .fill NUM_FREQS_ON_SCREEN, 0                        //; High bytes of raw bar heights
    RawBarHeightsLo:                .fill NUM_FREQS_ON_SCREEN, 0                        //; Low bytes of raw bar heights

    //; Channel-specific release rates
    ChannelReleaseHi:               .fill 3, 0                                          //; High bytes of release rates per channel
    ChannelReleaseLo:               .fill 3, 0                                          //; Low bytes of release rates per channel

    //; New buffer for smoothed bar heights
    SmoothedBarHeights:             .fill NUM_FREQS_ON_SCREEN, 0                        //; Smoothed bar heights
    
    //; Current bar heights for drawing
    .byte $00
    FrequencyBarHeights:          .fill NUM_FREQS_ON_SCREEN, 0  //; Final heights for display
    .byte $00

    //; SID register ghost copy for analysis
    SIDRegisterCopy:            .fill 32, 0    //; Copy of SID registers 
    PrevSIDRegisterCopy:        .fill 32, 0    //; Previous SID register values for gate detection

    //; Color lookup tables
    DarkColorLookup:            .byte $00, $0c, $00, $0e, $06, $09, $00, $08
                               .byte $02, $0b, $02, $00, $0b, $05, $06, $0c

    //; Color palettes for the visualization
    .var NUM_COLOR_PALETTES = 4
    ColorPaletteA:              .byte $09, $04, $0d, $01
    ColorPaletteB:              .byte $06, $0e, $07, $01
    ColorPaletteC:              .byte $02, $0a, $0d, $01
    ColorPaletteD:              .byte $0b, $0c, $0f, $01

    //; Pointers to color palettes
    ColorPalettePtr_Lo:         .byte <ColorPaletteA, <ColorPaletteB, <ColorPaletteC, <ColorPaletteD
    ColorPalettePtr_Hi:         .byte >ColorPaletteA, >ColorPaletteB, >ColorPaletteC, >ColorPaletteD

    //; Color mapping table based on bar height
    BarHeightToColorIndex:    .byte $ff                                      //;  baseline
                              .fill TOP_SPECTROMETER_PIXELHEIGHT, (i * 4) / TOP_SPECTROMETER_PIXELHEIGHT
                              .byte $03

    //; Color tables for the visualization
    BarColorsDark:              .fill TOP_SPECTROMETER_PIXELHEIGHT, $00  //; Darker colors for reflections
    BarColors:                  .fill TOP_SPECTROMETER_PIXELHEIGHT, $0b  //; Main colors for bars

    //; Calculate which IRQ should update the analyzer
    .var VISUALIZER_UPDATE_IRQ = 0
    .if (NumCallsPerFrame == 2) {
        .eval VISUALIZER_UPDATE_IRQ = 1
    } else {
        .if (NumCallsPerFrame == 3) {
            .eval VISUALIZER_UPDATE_IRQ = 2
        } else {
            .if (NumCallsPerFrame == 4) {
                .eval VISUALIZER_UPDATE_IRQ = 2
            } else {
                .if (NumCallsPerFrame == 5) {
                    .eval VISUALIZER_UPDATE_IRQ = 3
                } else {
                    .if (NumCallsPerFrame == 6) {
                        .eval VISUALIZER_UPDATE_IRQ = 3
                    }
                }
            }
        }
    }


    //; Character mapping for meter visualization
    .var METER_TO_CHAR_PADDING = TOP_SPECTROMETER_PIXELHEIGHT - 8

.align 256  
        .fill METER_TO_CHAR_PADDING, 224      //; Initial padding
    MeterToCharValues:
        .fill 8, i + 224 + 1              //; First 8 values increment
        .fill METER_TO_CHAR_PADDING, 224 + 9

EndLocalData:

//; Pre-calculate addresses for frequency tables
.var FreqHiTable = FreqTable + (0 * 256)
.var FreqLoTable = FreqTable + (1 * 256)

//; =============================================================================
//; CODE SEGMENT
//; =============================================================================
StartCodeSegment:

//; =============================================================================
//; MUSICPLAYER_Initialize() - Entry point for the music visualizer
//; =============================================================================
MUSICPLAYER_Initialize:
        sei                           //; Disable interrupts

        //; Wait for stable raster line
        bit $d011
        bpl *-3
        bit $d011
        bmi *-3

        //; Turn off screen temporarily during setup
        lda #$00
        sta $d011
        sta $d020

        //; Wait for another stable raster to ensure VIC is ready
        bit $d011
        bpl *-3
        bit $d011
        bmi *-3

        //; Set up the system
        jsr PreDemoSetup             //; Initialize system
        jsr NMIFix                   //; Fix NMI to prevent crashes

        //; Initialize VIC-II registers
        ldx #(NUM_VIC_INIT_VALUES - 1)
    InitVICRegisters:
        lda StartVICInitValues, x
        cmp #SKIP_VALUE
        beq SkipThisRegister
        sta $d000, x                 //; Set VIC register 
    SkipThisRegister:
        dex
        bpl InitVICRegisters

        //; Set up VIC memory banks
        lda #(63-BASE_BANK)          //; Calculate CIA value for bank
        sta $dd00
        lda #BASE_BANK
        sta $dd02

        //; Initialize the bar color values
        ldx #0
    InitBarColorsLoop:
        lda #$0b                     //; Default color
        ldy BarHeightToColorIndex, x
        bmi UseDefaultColor
        lda ColorPaletteA, y         //; Get color from palette
    UseDefaultColor:
        sta BarColors, x             //; Set main color
        tay
        lda DarkColorLookup, y       //; Look up darker version
        sta BarColorsDark, x         //; Set dark color for reflection
        inx
        cpx #TOP_SPECTROMETER_PIXELHEIGHT
        bne InitBarColorsLoop

        //; Clear screen and color memory
        ldx #$00
        lda #$20                     //; Space character
    ClearScreenLoop:
        sta $d800 + (0 * 256), x     //; Clear color RAM
        sta $d800 + (1 * 256), x
        sta $d800 + (2 * 256), x
        sta $d800 + (3 * 256), x
        sta SCREEN_ADDRESS + (0 * 256), x  //; Clear screen RAM
        sta SCREEN_ADDRESS + (1 * 256), x
        sta SCREEN_ADDRESS + (2 * 256), x
        sta SCREEN_ADDRESS + (3 * 256), x
        inx
        bne ClearScreenLoop

        //; Set up song and artist name colors
        ldx #79
    SetupTitleColorsLoop:
        lda #SONG_TITLE_COLOR
        sta $d800 + ((SONG_TITLE_SCREEN_LINE + 0) * 40), x
        lda #ARTIST_NAME_COLOR
        sta $d800 + ((ARTIST_NAME_SCREEN_LINE + 0) * 40), x
        dex
        bpl SetupTitleColorsLoop

        //; Wait for stable raster before turning screen back on
        bit $d011
        bpl *-3
        bit $d011
        bmi *-3

        //; Turn on the screen
        lda #$1b
        sta $d011

        //; Set up IRQ system
        lda #<MUSICPLAYER_IRQ0       //; Set IRQ vector
        sta $fffe
        lda #>MUSICPLAYER_IRQ0
        sta $ffff
        lda D012_Values              //; Set raster line
        sta $d012
        lda $d011
        and #$3f
        ora D011_Values
        sta $d011

        //; Enable raster IRQs
        lda #$01
        sta $d01a
        sta $d019

        //; Set final memory configuration
        lda #$35
        sta $01

        //; Initialize ADSR states for all channels
        ldx #3
        dex
    InitADSRLoop:
        lda #ADSR_RELEASE
        sta ChannelADSRState, x
        lda #0
        sta ChannelTargetHeights, x
        dex
        bpl InitADSRLoop

        cli                           //; Enable interrupts
    
        //; Initialize music and visualizer
        jsr MUSICPLAYER_SetupSong

        //; Main program loop - waits for visualization updates
    MainLoop:
        lda UpdateVisualizerSignal    //; Check if update is needed
        beq JustUpdateADSR            //; If no visual update, still update ADSR

        //; Update the visualization
        jsr MUSICPLAYER_SmoothBars    //; Apply smoothing to the bars
        jsr MUSICPLAYER_DrawBars      //; Draw the visualization

        lda #$00
        sta UpdateVisualizerSignal    //; Clear update flag

    JustUpdateADSR:
        jsr MUSICPLAYER_UpdateBarHeights  //; Apply ADSR envelope updates

        jmp MainLoop                  //; Continue looping

//; =============================================================================
//; MUSICPLAYER_NextIRQ() - Set up the next IRQ in the chain
//; =============================================================================
MUSICPLAYER_NextIRQ:
        ldx #$00                      //; IRQ counter (self-modified)

        //; Check if this is the IRQ that should update the visualizer
        ldy #$00
        cpx #VISUALIZER_UPDATE_IRQ
        bne SkipVisualizerUpdate
        iny                           //; Set update flag if this is the right IRQ
    SkipVisualizerUpdate:
        sty UpdateVisualizerSignal

        //; Move to next IRQ in the chain
        inx
        cpx #NumCallsPerFrame
        bne NotLastIRQ

        ldx #$00                      //; Reset to first IRQ if at end
    NotLastIRQ:
        stx MUSICPLAYER_NextIRQ + 1  //; Store for next time

        //; Set up for next IRQ
        lda D012_Values, x            //; Get raster line
        sta $d012
        lda $d011
        and #$3f
        ora D011_Values, x            //; Set high bit of raster line
        sta $d011

        //; Set appropriate vector based on which IRQ
        cpx #$00
        bne SetMusicOnlyIRQ

        //; Set vector for main IRQ
        lda #<MUSICPLAYER_IRQ0
        sta $fffe
        lda #>MUSICPLAYER_IRQ0
        sta $ffff
        rts

    SetMusicOnlyIRQ:
        //; Set vector for music-only IRQ
        lda #<MUSICPLAYER_IRQ_MusicOnly
        sta $fffe
        lda #>MUSICPLAYER_IRQ_MusicOnly
        sta $ffff
        rts

//; =============================================================================
//; MUSICPLAYER_IRQ0() - Main IRQ handler for music and visualization
//; =============================================================================
MUSICPLAYER_IRQ0:
        pha                           //; Save registers
        txa
        pha
        tya
        pha

        jsr MUSICPLAYER_PlayMusic     //; Play music

        //; Frame counter logic
        inc FrameCounter
        bne NotNewColorCycle

        //; Every 256 frames, update the color cycle counter
        inc Frame_256Counter

        //; Reset color update pointer
        lda #$00
        sta UpdateNextColorEntry + 1

    UpdateColorPalette:
        //; Update color palette every 256 frames
        ldx #$00
        inx
        cpx #NUM_COLOR_PALETTES
        bne NotLastPalette
        ldx #$00                      //; Loop back to first palette
    NotLastPalette:
        stx UpdateColorPalette + 1

        //; Update the active color palette pointers
        lda ColorPalettePtr_Lo, x
        sta ReadColorPaletteValue + 1
        lda ColorPalettePtr_Hi, x
        sta ReadColorPaletteValue + 2
    NotNewColorCycle:

    UpdateNextColorEntry:
        //; Gradually update colors over time
        lda #$00
        bmi DontUpdateColors
        tax

        //; Set up the color based on the palette and height
        lda #$0b                      //; Default color
        ldy BarHeightToColorIndex, x
        bmi UseBaseColor
    ReadColorPaletteValue:
        lda ColorPaletteA, y          //; Get color from current palette
    UseBaseColor:
        sta BarColors, x

        //; Look up darker version for reflection
        tay
        lda DarkColorLookup, y
        sta BarColorsDark, x

        //; Move to next color entry
        lda UpdateNextColorEntry + 1
        clc
        adc #$01
        cmp #$50
        bne NotFinishedColorUpdate
        lda #$ff                      //; Mark as done when all colors updated
    NotFinishedColorUpdate:
        sta UpdateNextColorEntry + 1
    DontUpdateColors:

    //; Update sprite positions based on sine wave
    UpdateSpritePositions:
        ldx #$00                      //; Self-modified index

        //; Read position from sine table
        lda SpriteSine, x
        
        //; Set X positions for all sprites, incrementing by 48 pixels each time
        .for (var i = 0; i < 7; i++) {
            sta $d000 + (i * 2)
            .if (i != 6) {
                clc
                adc #$30              //; 48 pixels between each sprite
            }
        }

        //; Set high bit of X position (for positions > 255)
        lda SpriteSine + 128, x
        sta $d010

    //; Animate sprites
    AnimateSprites:
        ldy #$00                      //; Self-modified animation frame

        //; Calculate sprite frame based on frame counter
        lda FrameCounter
        lsr                           //; Divide by 4 to slow animation
        lsr
        and #$07                      //; 8 animation frames
        ora #SPRITE_INDEX_START       //; Add base sprite index
        
        //; Set sprite pointers for all sprites
        .for (var i = 0; i < 7; i++) {
            sta SPRITE_POINTERS + i
        }

        //; Update sine wave position for next frame
        stx UpdateSpritePositions + 1

        //; Set up next IRQ
        jsr MUSICPLAYER_NextIRQ

        //; Acknowledge the interrupt
        lda #$01
        sta $d01a
        sta $d019

        //; Restore registers and return
        pla
        tay
        pla
        tax
        pla
        rti

//; =============================================================================
//; MUSICPLAYER_IRQ_MusicOnly() - Secondary IRQ handler (music only)
//; =============================================================================
MUSICPLAYER_IRQ_MusicOnly:
        pha                           //; Save registers
        txa
        pha
        tya
        pha

        jsr MUSICPLAYER_PlayMusic     //; Play music

        jsr MUSICPLAYER_NextIRQ       //; Set up next IRQ

        //; Acknowledge the interrupt
        lda #$01
        sta $d01a
        sta $d019

        //; Restore registers and return
        pla
        tay
        pla
        tax
        pla
        rti

//; =============================================================================
//; MUSICPLAYER_UpdateADSREnvelopes() - Update envelope state for all channels
//; =============================================================================

Mul7: .fill 3, i * 7
MUSICPLAYER_UpdateADSREnvelopes:
        //; Process each SID channel
        ldx #2                              //; Start with channel 2 (0-based indexing)
    ProcessChannelLoop:
        //; Check if channel is active by examining gate bit
        ldy Mul7, x
        lda SIDRegisterCopy + 4, y   //; Control register (D404/D40B/D412)
        and #1                              //; Isolate gate bit
        beq ChannelInRelease                //; If gate is off, always in release phase

        //; Channel is active (gate on), determine which ADSR phase it's in

        lda ChannelADSRState, x
        cmp #ADSR_ATTACK
        bne !skip+
        jmp ProcessAttackPhase
    !skip:
        cmp #ADSR_DECAY
        bne !skip+
        jmp ProcessDecayPhase
    !skip:
        cmp #ADSR_SUSTAIN
        bne !skip+
        jmp ProcessSustainPhase
    !skip:
        rts

    ChannelInRelease:
        //; Force channel to release phase if gate bit is off
        lda #ADSR_RELEASE
        sta ChannelADSRState, x
        jmp ProcessReleasePhase

    //; -------------------------------------------------------------------------
    //; Attack Phase Processing
    //; -------------------------------------------------------------------------
    ProcessAttackPhase:

        //; Find all frequency bars controlled by this channel
        stx !compVal+ + 1

        ldy #NUM_FREQS_ON_SCREEN - 1
    AttackPhaseLoop:
        lda ChannelToFreqMap, y
    !compVal:
        cmp #$ab
        bne NextFreqAttack

        //; This frequency bar belongs to current channel, update its height
        clc
        lda RawBarHeightsLo, y
        adc ChannelAttackLo, x
        sta RawBarHeightsLo, y
        lda RawBarHeightsHi, y
        adc ChannelAttackHi, x
        sta RawBarHeightsHi, y

        //; Check if reached maximum height
        lda RawBarHeightsHi, y
        cmp #<(TOP_SPECTROMETER_PIXELHEIGHT - 1)
        bcc NextFreqAttack

    ReachedPeak:
        //; This bar has reached peak, move channel to decay phase
        lda #ADSR_DECAY
        sta ChannelADSRState, x

        //; Set exact peak height
        lda #<(TOP_SPECTROMETER_PIXELHEIGHT - 1)
        sta RawBarHeightsHi, y
        lda #0
        sta RawBarHeightsLo, y

    NextFreqAttack:
        dey
        bpl AttackPhaseLoop
        jmp NextChannel                     //; Process next channel

    //; -------------------------------------------------------------------------
    //; Decay Phase Processing
    //; -------------------------------------------------------------------------
    ProcessDecayPhase:
        //; Calculate target sustain level from SID register
        ldy Mul7, x
        lda SIDRegisterCopy + 6, y  //; Sustain/Release register (D406/D40D/D414)
        lsr                                 //; Shift right 4 times to get sustain value (high nibble)
        lsr
        lsr
        lsr
        tay
        lda SustainConversion, y            //; Convert to bar height
        sta ChannelTargetHeights, x

        //; Get all frequency bars controlled by this channel
        stx !compVal+ + 1

        ldy #NUM_FREQS_ON_SCREEN - 1
    DecayPhaseLoop:
        lda ChannelToFreqMap, y
    !compVal:
        cmp #$ab
        bne NextFreqDecay

        //; Decrease height by decay rate
        sec
        lda RawBarHeightsLo, y
        sbc ChannelDecayLo, x
        sta RawBarHeightsLo, y
        lda RawBarHeightsHi, y
        sbc ChannelDecayHi, x
        sta RawBarHeightsHi, y

        //; Check if reached sustain level
        cmp ChannelTargetHeights, x
        bcc ReachedSustain                  //; If height < target, we've gone below
        bne NextFreqDecay                   //; If height > target, continue decay
        lda RawBarHeightsLo, y
        beq ReachedSustain                  //; If height == target, we're exactly there

        jmp NextFreqDecay

        .byte $12, $01, $09, $13, $14, $0c, $09, $0e

    ReachedSustain:
        //; This bar has reached sustain level, move channel to sustain phase
        lda #ADSR_SUSTAIN
        sta ChannelADSRState, x

        //; Set exact sustain height
        lda ChannelTargetHeights, x
        sta RawBarHeightsHi, y
        lda #0
        sta RawBarHeightsLo, y

    NextFreqDecay:
        dey
        bpl DecayPhaseLoop
        jmp NextChannel                     //; Process next channel

    //; -------------------------------------------------------------------------
    //; Sustain Phase Processing
    //; -------------------------------------------------------------------------
    ProcessSustainPhase:
        //; During Sustain phase, heights remain constant until gate bit is cleared
        //; Check if sustain level changed (e.g., modulated effect)
        ldy Mul7, x
        lda SIDRegisterCopy + 6, y   //; Sustain/Release register
        lsr                                 //; Shift to get just sustain value
        lsr
        lsr
        lsr
        tay
        lda SustainConversion, y            //; Convert to height
        
        //; Has sustain level changed? If so, update target and return to decay phase
        cmp ChannelTargetHeights, x
        beq SustainLevelUnchanged
        
        //; Sustain level changed - update target and reprocess
        sta ChannelTargetHeights, x
        
        //; Determine whether to move to attack or decay phase based on new level
        stx !compVal+ + 1

        ldy #NUM_FREQS_ON_SCREEN - 1
    CheckNewSustainLevel:
        lda ChannelToFreqMap, y
    compVal:
        cmp #$ab
        bne NextBarSustainCheck
        
        lda RawBarHeightsHi, y
        cmp ChannelTargetHeights, x
        bcc MoveToDuringAttack   //; Current height < new sustain = attack
        bne MoveToDuringDecay    //; Current height > new sustain = decay
        
    NextBarSustainCheck:
        dey
        bpl CheckNewSustainLevel
        jmp SustainLevelUnchanged
        
    MoveToDuringAttack:
        lda #ADSR_ATTACK
        sta ChannelADSRState, x
        jmp NextChannel
        
    MoveToDuringDecay:
        lda #ADSR_DECAY
        sta ChannelADSRState, x
        jmp NextChannel
        
    SustainLevelUnchanged:
        //; Stay in sustain phase
        jmp NextChannel

    //; -------------------------------------------------------------------------
    //; Release Phase Processing
    //; -------------------------------------------------------------------------
    ProcessReleasePhase:
        //; Release phase - all bars for this channel decrease until they reach zero
        stx !compVal+ + 1

        ldy #NUM_FREQS_ON_SCREEN - 1
    ReleasePhaseLoop:
        lda ChannelToFreqMap, y
    !compVal:
        cmp #$ab
        bne NextFreqRelease
        
        //; This frequency bar belongs to current channel, decrease height
        sec
        lda RawBarHeightsLo, y
        sbc ChannelReleaseLo, x
        sta RawBarHeightsLo, y
        lda RawBarHeightsHi, y
        sbc ChannelReleaseHi, x
        
        //; Check if reached zero (or went negative)
        bmi ZeroHeight
        sta RawBarHeightsHi, y
        ora RawBarHeightsLo, y
        beq ZeroHeight
        jmp NextFreqRelease
        
    ZeroHeight:
        //; Set height to exactly zero
        lda #0
        sta RawBarHeightsHi, y
        sta RawBarHeightsLo, y
        
    NextFreqRelease:
        dey
        bpl ReleasePhaseLoop

    NextChannel:
        dex
        bmi !noloop+
        jmp ProcessChannelLoop
    !noloop:
        rts

//; =============================================================================
//; MUSICPLAYER_UpdateBarHeights() - Apply decay to all bar heights (per frame)
//; =============================================================================
MUSICPLAYER_UpdateBarHeights:
        //; First update ADSR envelopes for all channels
        jsr MUSICPLAYER_UpdateADSREnvelopes

        //; Convert raw heights to display heights using sine wave (for smoother visual)
        ldx #NUM_FREQS_ON_SCREEN - 1
    ConvertHeightsLoop:
        lda RawBarHeightsHi, x
        tay
        lda SoundBarSine, y
        sta FrequencyBarHeights, x
        dex
        bpl ConvertHeightsLoop

        rts

//; =============================================================================
//; MUSICPLAYER_SmoothBars() - Apply smoothing to create more natural visualization
//; =============================================================================
MUSICPLAYER_SmoothBars:

        ldx #0
    SmoothingLoop:

        lda FrequencyBarHeights - 1, x  //; Left neighbor
        clc
        adc FrequencyBarHeights + 1, x  //; Right neighbor
        lsr
        clc
        adc FrequencyBarHeights, x      //; Current bar
        lsr
        sta SmoothedBarHeights, x

        inx
        cpx #NUM_FREQS_ON_SCREEN
        bne SmoothingLoop
        
        rts

//; =============================================================================
//; MUSICPLAYER_DrawBars() - Draw the spectrometer bars on screen
//; =============================================================================
MUSICPLAYER_DrawBars:
        //; Draw each frequency bar with its reflection
        .for (var i = 0; i < NUM_FREQS_ON_SCREEN; i++) {
            //; Check if this bar changed since last frame
            ldx SmoothedBarHeights + i
            cpx PrevFrame_BarHeights + i
            beq !skipBarUpdate+

        !updateBar:
            //; Store new height for next comparison
            stx PrevFrame_BarHeights + i

            //; Draw the main spectrometer bars
            .for (var line = 0; line < TOP_SPECTROMETER_HEIGHT; line++) {
                lda MeterToCharValues - METER_TO_CHAR_PADDING + (line * 8), x
                sta SCREEN_ADDRESS + ((SPECTROMETER_START_LINE + line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
            }

            //; Draw the reflection effect (use division by 3 to make it smaller)
            lda Div3Table, x
            tay
            .for (var line = 0; line < BOTTOM_SPECTROMETER_HEIGHT; line++) {
                lda MeterToCharValues - 17 + (line * 8), y
                clc
                adc ReflectionOffset + 1     //; Add animation offset
                sta SCREEN_ADDRESS + ((SPECTROMETER_START_LINE + TOP_SPECTROMETER_HEIGHT + BOTTOM_SPECTROMETER_HEIGHT - 1 - line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
            }

        !skipBarUpdate:

            //; Update the bar colors if changed
            lda BarColors, x
            cmp PrevFrame_BarColors + i
            beq !skipColorUpdate+
            sta PrevFrame_BarColors + i

            //; Get the channel that controls this bar
            ldy ChannelToFreqMap + i
            cpy #ADSR_RELEASE
            beq !useStandardColor+
            
            //; Use ADSR phase-specific color (optional enhancement)
            lda ChannelADSRState, y
            tax
            lda ADSRPhaseColors, x
            jmp !setColors+
            
        !useStandardColor:
            //; Use standard color
            lda BarColors, x
            
        !setColors:
            //; Set colors for main bars
            .for (var line = 0; line < TOP_SPECTROMETER_HEIGHT; line++) {
                sta $d800 + ((SPECTROMETER_START_LINE + line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
            }

            //; Set darker colors for reflection
            tax
            lda DarkColorLookup, x
            .for (var line = 0; line < BOTTOM_SPECTROMETER_HEIGHT; line++) {
                sta $d800 + ((SPECTROMETER_START_LINE + TOP_SPECTROMETER_HEIGHT + BOTTOM_SPECTROMETER_HEIGHT - 1 - line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
            }

        !skipColorUpdate:
        }

    //; Animate the reflection effect
    ReflectionOffset:
        lda #10
        sec
        sbc #10
        bne ReflectionOffsetOK
        lda #20                      //; Loop back to initial value
    ReflectionOffsetOK:
        sta ReflectionOffset + 1

        rts

//; =============================================================================
//; MUSICPLAYER_PlayMusic() - Handle SID music playback and register capture
//; =============================================================================
MUSICPLAYER_PlayMusic:
        //; Save current memory configuration
        lda $01
        pha
        
        //; Switch to bank with BASIC/KERNAL ROM visible
        lda #$30
        sta $01

        //; Call the music player routine
        jsr SIDPlay

        //; Save current SID registers to previous buffer, then update current
        .for (var i=24; i >= 0; i--) {
            lda SIDRegisterCopy + i
            sta PrevSIDRegisterCopy + i      //; Save current as previous
            lda $d400 + i
            sta SIDRegisterCopy + i          //; Update current
        }
        
        //; Restore memory configuration
        pla
        sta $01

        //; Write registers back to SID
        #if SID_REGISTER_REORDER_AVAILABLE
            //; Use custom register write order if available
            .for (var i = 0; i < SIDRegisterCount; i++) {
                lda SIDRegisterCopy + SIDRegisterOrder.get(i)
                sta $d400 + SIDRegisterOrder.get(i)
            }
        #else
            //; Standard register write order
            .for (var i = 0; i <= 24; i++) {
                lda SIDRegisterCopy + i
                sta $d400 + i
            }
        #endif

        //; Analyze SID registers and update visualizer
        jmp MUSICPLAYER_AnalyzeSIDRegisters

//; =============================================================================
//; MUSICPLAYER_AnalyzeSIDRegisters() - Analyze SID registers for visualization
//; =============================================================================
MUSICPLAYER_AnalyzeSIDRegisters:
        //; Process each of the three SID channels
        .for (var channel = 0; channel < 3; channel++) {
            //; Check if voice is active (gate bit set and not in noise mode)
            lda SIDRegisterCopy + (channel * 7) + 4  //; Control register
            bmi !skipChannelJMP+                        //; Skip if in noise mode
            and #1                                   //; Check gate bit
            bne !processChannel+                     //; Process if gate is on
        !skipChannelJMP:
            jmp !skipChannel+                        //; Skip if gate is off

        !processChannel:
            //; Get the frequency and map it to a bar position
            ldy SIDRegisterCopy + (channel * 7) + 1  //; hi-freq

            cpy #4
            bcc !useLoFreqTable+            //; Use lo-freq table if hi-freq < 4

        !useHiFreqTable:
            //; For higher frequencies, use the hi-freq table
            ldx FreqHiTable, y
            jmp !gotBarPosition+

        !useLoFreqTable:
            //; For lower frequencies, combine lo and hi values
            ldx SIDRegisterCopy + (channel * 7) + 0  //; lo-freq
            lda LoFreqToLookupTable, x
            ora HiFreqToLookupTable, y
            tay
            ldx FreqLoTable, y
            
        !gotBarPosition:
            //; Get the attack/decay values
            ldy SIDRegisterCopy + (channel * 7) + 5  //; attack(hi)/decay(lo)
            
            //; Set up attack rate for this channel
            lda AttackConversionHi, y
            sta ChannelAttackHi + channel
            lda AttackConversionLo, y
            sta ChannelAttackLo + channel

            //; Set up decay rate for this channel
            lda DecayConversionHi, y
            sta ChannelDecayHi + channel
            lda DecayConversionLo, y
            sta ChannelDecayLo + channel

            //; Get the sustain/release values
            ldy SIDRegisterCopy + (channel * 7) + 6  //; sustain(hi)/release(lo)
            
            //; Set up release rate for this channel
            lda ReleaseConversionHi, y
            sta ChannelReleaseHi + channel
            lda ReleaseConversionLo, y
            sta ChannelReleaseLo + channel

            //; Check for note-on transition (gate bit turning from 0 to 1)
            lda PrevSIDRegisterCopy + (channel * 7) + 4  //; Previous control register
            and #1                                       //; Check previous gate bit
            bne !skipNoteOn+                             //; If gate was already on, skip
            
            lda #ADSR_ATTACK
            sta ChannelADSRState + channel
            
            //; Clear ALL previous mappings for this channel to ensure only one bar per channel
            ldy #NUM_FREQS_ON_SCREEN - 1
        !clearMappingsLoop:
            lda ChannelToFreqMap, y
            cmp #channel
            bne !nextBar+
            
            //; Clear this mapping
            lda #ADSR_RELEASE
            sta ChannelToFreqMap, y
            
            //; Also reset the height for this bar
            lda #0
            sta RawBarHeightsHi, y
            sta RawBarHeightsLo, y
            
        !nextBar:
            dey
            bpl !clearMappingsLoop-

            //; Now assign the precise frequency bar for this channel
            lda #channel
            sta ChannelToFreqMap, x  //; X register contains the frequency bar position
            
            //; Initialize bar height to zero for this new note
            lda #0
            sta RawBarHeightsHi, x
            sta RawBarHeightsLo, x
            
            jmp !skipChannel+        //; Skip the rest of the processing since we've handled everything

        !skipNoteOn:
            //; For ongoing notes, we only update the mapping if needed
            lda ChannelADSRState + channel
            cmp #ADSR_ATTACK
            beq !updateBarMapping+   //; Always update during Attack
            
            lda SustainConversion, y //; Get sustain height 
            cmp RawBarHeightsHi, x   //; Compare to current height
            bcc !skipChannel+        //; Skip if current is higher

        !updateBarMapping:
            //; Record which channel controls this frequency bar
            lda #channel
            sta ChannelToFreqMap, x

        !skipChannel:
        }
        rts

//; =============================================================================
//; MUSICPLAYER_SetupSong() - Initialize music and display information
//; =============================================================================
MUSICPLAYER_SetupSong:

        //; Clear SID registers
        ldy #24
        lda #$00
    ClearSIDRegisters:
        sta $d400, y
        dey
        bpl ClearSIDRegisters

        //; Display song title
        ldy #SONG_TITLE_LENGTH - 1
    DisplaySongTitle:
        lda SongData_Title, y
        sta SCREEN_ADDRESS + ((SONG_TITLE_SCREEN_LINE + 0) * 40) + SONG_TITLE_PADDING, y
        ora #$80                      //; Reverse character for bottom row
        sta SCREEN_ADDRESS + ((SONG_TITLE_SCREEN_LINE + 1) * 40) + SONG_TITLE_PADDING, y
        dey
        bpl DisplaySongTitle

        //; Display artist name
        ldy #ARTIST_NAME_LENGTH - 1
    DisplayArtistName:
        lda SongData_Artist, y
        sta SCREEN_ADDRESS + ((ARTIST_NAME_SCREEN_LINE + 0) * 40) + ARTIST_NAME_PADDING, y
        ora #$80                      //; Reverse character for bottom row
        sta SCREEN_ADDRESS + ((ARTIST_NAME_SCREEN_LINE + 1) * 40) + ARTIST_NAME_PADDING, y
        dey
        bpl DisplayArtistName

        //; Initialize the SID player
        lda #$00                      //; Start with song #0
        jmp SIDInit

//; =============================================================================
//; PreDemoSetup() - Initialize system for stable raster IRQs
//; =============================================================================
.align 256
PreDemoSetup:
        //; From Spindle by lft, www.linusakesson.net/software/spindle/
        //; Prepare CIA #1 timer B to compensate for interrupt jitter.
        //; Also initialize d01a and dc02.

        //; Wait for stable raster
        bit $d011
        bmi *-3

        bit $d011
        bpl *-3

        ldx $d012
        inx
    ResyncRaster:
        cpx $d012
        bne *-3

        ldy #0
        sty $dc07
        lda #62
        sta $dc06
        iny
        sty $d01a
        dey
        dey
        sty $dc02
        cmp (0,x)
        cmp (0,x)
        cmp (0,x)
        lda #$11
        sta $dc0f
        txa
        inx
        inx
        cmp $d012
        bne ResyncRaster

        //; Set up IRQ system
        lda #$7f
        sta $dc0d
        sta $dd0d
        lda $dc0d
        lda $dd0d

        //; Wait for stable raster
        bit $d011
        bpl *-3
        bit $d011
        bmi *-3

        //; Enable raster IRQs
        lda #$01
        sta $d01a

        rts

//; =============================================================================
//; Include necessary system files
//; =============================================================================
.import source "..\INC\NMIFix.asm"
.import source "..\INC\RasterLineTiming.asm"

EndCodeSegment:

//; =============================================================================
//; DATA SEGMENTS - Character and sprite data
//; =============================================================================

//; Water reflection sprites
* = WATERSPRITES_ADDRESS
StartWaterSprites:
    .fill watersprites_map.getSize(), watersprites_map.get(i)
EndWaterSprites:

//; Character set for bars
* = CHARSET_ADDRESS
StartCharSet:
    .fill charset_map.getSize(), charset_map.get(i)
EndCharSet:

//; =============================================================================
//; Memory usage summary
//; =============================================================================
    .print "* $" + toHexString(StartCodeSegment) + "-$" + toHexString(EndCodeSegment - 1) + " Code Segment"
    .print "* $" + toHexString(StartLocalData) + "-$" + toHexString(EndLocalData - 1) + " Local Data"
    .print "* $" + toHexString(StartCharSet) + "-$" + toHexString(EndCharSet - 1) + " Character Set"
    .print "* $" + toHexString(StartWaterSprites) + "-$" + toHexString(EndWaterSprites - 1) + " Water Sprites"