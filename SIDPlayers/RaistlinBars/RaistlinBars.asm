//; (c) 2018, Raistlin of Genesis*Project

* = PlayerADDR

	jmp MUSICPLAYER_Go

.var spritesine_bin = LoadBinary("SpriteSine.bin")
.var freqtable_bin = LoadBinary("FreqTable.bin")
.var soundbarsine_bin = LoadBinary("SoundBarSine.bin")
.var charset_map = LoadBinary("CharSet.map")
.var watersprites_map = LoadBinary("WaterSprites.map")

//; Defines -------------------------------------------------------------------------------------------------------

	.var NUM_FREQS_ON_SCREEN				= 40

	.var SongNameColour						= $01
	.var SongNameScreenLine					= 0
	.var SongNameLength						= min(SIDName.size(), 40)
	.var SongNamePadding					= (40 - SongNameLength) / 2

	.var ArtistNameColour					= $0f
	.var ArtistNameScreenLine				= 3
	.var ArtistNameLength					= min(SIDAuthor.size(), 40)
	.var ArtistNamePadding					= (40 - ArtistNameLength) / 2

	.var TopSpectrometerHeight				= 10
	.var BottomSpectrometerHeight			= 3
	.var SPECTROMETER_StartLine				= 25 - (TopSpectrometerHeight + BottomSpectrometerHeight)

	.var BaseBank							= 3 //; 0=0000-3fff, 1=4000-7fff, 2=8000-bfff, 3=c000-ffff
	.var Base_BankAddress					= (BaseBank * $4000)	
	.var ScreenBank							= 14 //; Bank+[3800,3bff]
	.var CharBankFont						= 6 //; Bank+[3000,37ff] //; We only used half the font...
	.var ScreenAddress						= (Base_BankAddress + (ScreenBank * $400))
	.var SpriteVals							= (ScreenAddress + $3F8 + 0)
	.var ADDR_CharSet						= (Base_BankAddress + (CharBankFont * $800))
	.var StartSpriteIndex					= $80
	.var ADDR_WaterSprites					= (Base_BankAddress + (StartSpriteIndex * $40))

//; StartLocalData -------------------------------------------------------------------------------------------------------
StartLocalData:

	.var SKIPPValue = $e1

	.var NumVICInitValues = EndVICInitValues - StartVICInitValues

	StartVICInitValues:
		.byte $10										//; D000: $d000
		.byte $e5										//; D001: Sprite0Y
		.byte $40										//; D002: Sprite1X
		.byte $e5										//; D003: Sprite1Y
		.byte $70										//; D004: Sprite2X
		.byte $e5										//; D005: Sprite2Y
		.byte $a0										//; D006: Sprite3X
		.byte $e5										//; D007: Sprite3Y
		.byte $d0										//; D008: Sprite4X
		.byte $e5										//; D009: Sprite4Y
		.byte $00										//; D00A: Sprite5X
		.byte $e5										//; D00B: Sprite5Y
		.byte $30										//; D00C: Sprite6X
		.byte $e5										//; D00D: Sprite6Y
		.byte $00										//; D00E: Sprite7X
		.byte $00										//; D00F: Sprite7Y
		.byte $60										//; D010: SpriteXMSB
		.byte SKIPPValue								//; D011
		.byte SKIPPValue								//; D012
		.byte SKIPPValue								//; D013: LightPenX
		.byte SKIPPValue								//; D014: LightPenY
		.byte $7f										//; D015: SpriteEnable
		.byte $08										//; D016: ScreenScroll/MC
		.byte $00										//; D017: SpriteDoubleHeight
		.byte ((ScreenBank * 16) + (CharBankFont * 2))	//; D018: Screen/CharSet
		.byte SKIPPValue								//; D019
		.byte SKIPPValue								//; D01A
		.byte $00										//; D01B: SpriteDrawPriority
		.byte $00										//; D01C: SpriteMulticolourMode
		.byte $7f										//; D01D: SpriteDoubleWidth
		.byte $00										//; D01E: SpriteSpriteCollision
		.byte $00										//; D01F: SpriteBackgroundCollision
		.byte $00										//; D020: BorderColour
		.byte $00										//; D021: ScreenColour
		.byte $00										//; D022: MultiColour0 (and ExtraBackgroundColour0)
		.byte $00										//; D023: MultiColour1 (and ExtraBackgroundColour1)
		.byte $00										//; D024: ExtraBackgroundColour2
		.byte $00										//; D025: SpriteExtraColour0
		.byte $00										//; D026: SpriteExtraColour1
		.byte $00										//; D027: Sprite0Colour
		.byte $00										//; D028: Sprite1Colour
		.byte $00										//; D029: Sprite2Colour
		.byte $00										//; D02A: Sprite3Colour
		.byte $00										//; D02B: Sprite4Colour
		.byte $00										//; D02C: Sprite5Colour
		.byte $00										//; D02D: Sprite6Colour
		.byte $00										//; D02E: Sprite7Colour
	EndVICInitValues:

	SongData_SongName:			.text SIDName.substring(0, min(SIDName.size(), 40))
	SongData_ArtistName:		.text SIDAuthor.substring(0, min(SIDAuthor.size(), 40))

	UpdateSpectrometerSignal:	.byte $00

	FrameOf256:					.byte $00
	Frame_256Counter:			.byte $00

	LastFrame_bBMeterValue:		.fill 40, 255
	LastFrame_Colours:			.fill 40, 255

	.align 128

	Div3:						.fill 128, i / 3
	SoundbarSine:				.fill soundbarsine_bin.getSize(), soundbarsine_bin.get(i)

	.align 256

	SpriteSine:					.fill spritesine_bin.getSize(), spritesine_bin.get(i)
	FreqTable:					.fill freqtable_bin.getSize(), freqtable_bin.get(i)

	LoFreqToLookupTable:		.fill 256, i / 4
	HiFreqToLookupTable:		.fill 4, i * 64

	SustainConversion:			.fill 256, floor(i / 16) * 6 + 32 + 3

	ReleaseConversionHi:		.fill 256, (mod(i, 16) * 64 + 768) / 256
	ReleaseConversionLo:		.fill 256, mod(mod(i, 16) * 64 + 768, 256)
								
								.byte 0, 0
	MeterChannel:				.fill NUM_FREQS_ON_SCREEN, 0
								.byte 0, 0


								.byte 0, 0
	MeterTempHi:				.fill NUM_FREQS_ON_SCREEN, 0
								.byte 0, 0

								.byte 0, 0
	MeterTempLo:				.fill NUM_FREQS_ON_SCREEN, 0
								.byte 0, 0

	MeterReleaseHiPerChannel:	.fill 3, 0
	MeterReleaseLoPerChannel:	.fill 3, 0


	MeterColourValues_Darker:	.fill 80, $00
	MeterColourValues:			.fill 80, $0b

	dBMeterValue:				.fill NUM_FREQS_ON_SCREEN,0
	SIDGhostbytes:				.fill 32,0

	DarkColourLookup:			.byte $00, $0c, $00, $0e, $06, $09, $00, $08
								.byte $02, $0b, $02, $00, $0b, $05, $06, $0c

	.var NumColourSets = 4
	MeterColourSetA:			.byte $09, $04, $0d, $01
	MeterColourSetB:			.byte $06, $0e, $07, $01
	MeterColourSetC:			.byte $02, $0a, $0d, $01
	MeterColourSetD:			.byte $0b, $0c, $0f, $01

	MeterColourSetPtr_Lo:		.byte <MeterColourSetA, <MeterColourSetB, <MeterColourSetC, <MeterColourSetD
	MeterColourSetPtr_Hi:		.byte >MeterColourSetA, >MeterColourSetB, >MeterColourSetC, >MeterColourSetD

	MeterColourValueOffsets:	.fill 2, $ff											//; +  2 =  2
								.byte $00, $ff, $00, $ff, $00, $ff, $00, $ff, $00, $ff	//; + 10 = 12
								.fill 5, $00											//; +  5 = 17
								.byte $01, $00, $01, $00, $01, $00, $01, $00, $01, $00	//; + 10 = 27
								.byte $01, $00, $01, $00, $01, $00, $01, $00, $01, $00	//; + 10 = 37
								.fill 5, $01											//; +  5 = 42
								.byte $02, $01, $02, $01, $02, $01, $02, $01, $02, $01	//; + 10 = 52
								.byte $02, $01, $02, $01, $02, $01, $02, $01, $02, $01	//; + 10 = 62
								.fill 4, $02											//; +  4 = 66
								.byte $03, $02, $03, $02, $03, $02, $03, $02, $03, $02	//; + 10 = 76
								.fill 4, $03											//; +  4 = 80


	.var SpectraliserUpdateIRQ = 0
	.if (NumCallsPerFrame == 2) {
	    .eval SpectraliserUpdateIRQ = 1
	} else {
		.if (NumCallsPerFrame == 3) {
		    .eval SpectraliserUpdateIRQ = 2
		} else {
			.if (NumCallsPerFrame == 4) {
				.eval SpectraliserUpdateIRQ = 2
			} else {
				.if (NumCallsPerFrame == 5) {
					.eval SpectraliserUpdateIRQ = 3
				} else {
					.if (NumCallsPerFrame == 6) {
						.eval SpectraliserUpdateIRQ = 3
					}
				}
			}
		}
	}

	.var MeterToChar_PrePadding = (TopSpectrometerHeight * 8) - 10

.align 256	
		.fill MeterToChar_PrePadding, 224
	MeterToCharValues:
		.fill 8, i + 224 + 1
		.fill (TopSpectrometerHeight * 8), 224 + 9

EndLocalData:

.var FreqHiTable = FreqTable + (0 * 256)
.var FreqLoTable = FreqTable + (1 * 256)

StartCodeSegment:

//; MUSICPLAYER_Go() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_Go:

		sei

		bit	$d011
		bpl	*-3
		bit	$d011
		bmi	*-3

		lda #$00
		sta $d011
		sta $d020

		bit	$d011
		bpl	*-3
		bit	$d011
		bmi	*-3

		jsr PreDemoSetup

		jsr NMIFix

		ldx #(NumVICInitValues - 1)
	SetupD000Values:
		lda StartVICInitValues, x
		cmp #SKIPPValue
		beq SkipThisOne
		sta $d000, x
	SkipThisOne:
		dex
		bpl SetupD000Values

		lda #(63-BaseBank)
		sta $dd00
		lda #BaseBank
		sta $dd02

	//; Init the meter colour values
		ldx #79
	InitMeterColourValuesLoop:
		lda #$0b
		ldy MeterColourValueOffsets, x
		bmi UseTheBaseLineColour
		lda MeterColourSetA, y
	UseTheBaseLineColour:
		sta MeterColourValues, x
		tay
		lda DarkColourLookup, y
		sta MeterColourValues_Darker, x
		dex
		bpl InitMeterColourValuesLoop

		ldx #$00
		lda #$20
	ClearLoop:
		sta $d800 + (0 * 256), x
		sta $d800 + (1 * 256), x
		sta $d800 + (2 * 256), x
		sta $d800 + (3 * 256), x
		sta ScreenAddress + (0 * 256), x
		sta ScreenAddress + (1 * 256), x
		sta ScreenAddress + (2 * 256), x
		sta ScreenAddress + (3 * 256), x
		inx
		bne ClearLoop

		ldx #79
	FillDisplayNameColoursLoop:
		lda #SongNameColour
		sta $d800 + ((SongNameScreenLine + 0) * 40), x
		lda #ArtistNameColour
		sta $d800 + ((ArtistNameScreenLine + 0) * 40), x
		dex
		bpl FillDisplayNameColoursLoop

		bit	$d011
		bpl	*-3
		bit	$d011
		bmi	*-3

		lda #$1b
		sta $d011

		lda #<MUSICPLAYER_IRQ0
		sta $fffe
		lda #>MUSICPLAYER_IRQ0
		sta $ffff
		lda D012_Values
		sta $d012
		lda $d011
		and #$3f
		ora D011_Values
		sta $d011

		lda #$01
		sta $d01a
		sta $d019

		lda #$35
		sta $01

		cli
	
		jsr MUSICPLAYER_SetupNewSong

	LoopForever:

		lda UpdateSpectrometerSignal
		beq LoopForever

		jsr MUSICPLAYER_DrawSpectrum

		lda #$00
		sta UpdateSpectrometerSignal

		jsr MUSICPLAYER_Spectrometer_PerFrame

		jmp LoopForever

MUSICPLAYER_NextIRQ:
		ldx #$00

		ldy #$00
		cpx #SpectraliserUpdateIRQ
		bne !skip+
		iny
	!skip:
		sty UpdateSpectrometerSignal

		inx

		cpx #NumCallsPerFrame
		bne NotFinalIRQ

		ldx #$00
	NotFinalIRQ:
		stx MUSICPLAYER_NextIRQ + 1

		lda D012_Values, x
		sta $d012
		lda $d011
		and #$3f
		ora D011_Values, x
		sta $d011

		cpx #$00
		bne MoreToPlay

		lda #<MUSICPLAYER_IRQ0
		sta $fffe
		lda #>MUSICPLAYER_IRQ0
		sta $ffff
		rts

	MoreToPlay:

		lda #<MUSICPLAYER_IRQ_JustMusic
		sta $fffe
		lda #>MUSICPLAYER_IRQ_JustMusic
		sta $ffff
		rts

//; MUSICPLAYER_IRQ0() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_IRQ0:

		pha
		txa
		pha
		tya
		pha

		jsr MUSICPLAYER_PlayMusic

		inc FrameOf256
		bne Not256thFrame
		inc Frame_256Counter

		lda #$00
		sta DoNextColourSet + 1
	ColourSet:
		ldx #$00
		inx
		cpx #NumColourSets
		bne NotLastColourSet
		ldx #$00
	NotLastColourSet:
		stx ColourSet + 1

		lda MeterColourSetPtr_Lo, x
		sta MeterColourSetReadLDA + 1
		lda MeterColourSetPtr_Hi, x
		sta MeterColourSetReadLDA + 2
	Not256thFrame:

	DoNextColourSet:
		lda #$00
		bmi DontUpdateColourSet
		tax

		lda #$0b
		ldy MeterColourValueOffsets, x
		bmi UseBaseLineColourB
	MeterColourSetReadLDA:
		lda MeterColourSetA, y
	UseBaseLineColourB:
		sta MeterColourValues, x

		tay
		lda DarkColourLookup, y
		sta MeterColourValues_Darker, x

		lda DoNextColourSet + 1
		clc
		adc #$01
		cmp #$50
		bne NotLastEntry
		lda #$ff
	NotLastEntry:
		sta DoNextColourSet + 1
	DontUpdateColourSet:

	SpriteMovementX:
		ldx #$00

		lda SpriteSine, x
		.for (var Index = 0; Index < 7; Index++)
		{
			sta $d000 + (Index * 2)
			.if (Index != 6)
			{
				clc
				adc #$30
			}
		}

		lda SpriteSine + 128, x
		sta $d010

	//; Sprite Animation
		ldy #$00
	SpriteAnimIndex:
		lda FrameOf256
		lsr
		lsr
		and #$07
		ora #StartSpriteIndex
		.for (var Index = 0; Index < 7; Index++)
		{
			sta SpriteVals + Index
		}

		stx SpriteMovementX + 1

		jsr MUSICPLAYER_NextIRQ

		lda #$01
		sta $d01a
		sta $d019

		pla
		tay
		pla
		tax
		pla

		rti

//; MUSICPLAYER_IRQ_JustMusic() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_IRQ_JustMusic:

		pha
		txa
		pha
		tya
		pha

		jsr MUSICPLAYER_PlayMusic

		jsr MUSICPLAYER_NextIRQ

		lda #$01
		sta $d01a
		sta $d019

		pla
		tay
		pla
		tax
		pla

		rti

//; MUSICPLAYER_Spectrometer_PerFrame() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_Spectrometer_PerFrame:

		ldx #NUM_FREQS_ON_SCREEN - 1

	!loop:
		ldy MeterChannel, x
		sec
		lda MeterTempLo, x
		sbc MeterReleaseLoPerChannel, y
		sta MeterTempLo, x
		lda MeterTempHi, x
		sbc MeterReleaseHiPerChannel, y
		bpl !good+
		lda #$00
		sta MeterTempLo, x
	!good:
		sta MeterTempHi, x
		tay
		lda SoundbarSine, y
		sta dBMeterValue, x

		dex
		bpl !loop-

		rts

//; MUSICPLAYER_Spectrometer_PerPlay() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_Spectrometer_PerPlay:

		.for (var ChannelIndex = 0; ChannelIndex < 3; ChannelIndex++)
		{
			lda SIDGhostbytes + (ChannelIndex * 7) + 4
			bmi !skipUpdate+
			and #1
			bne !continue+
		!skipUpdate:
			jmp NoUpdate

		!continue:

			ldy SIDGhostbytes + (ChannelIndex * 7) + 1	//; hi-freq

			cpy #4
			bcc Check_FreqLoTable

		Check_FreqHiTable:
			ldx FreqHiTable, y
			jmp GotFreq

		Check_FreqLoTable:
			ldx SIDGhostbytes + (ChannelIndex * 7) + 0	//; lo-freq
			lda LoFreqToLookupTable, x
			ora HiFreqToLookupTable, y
			tay
			ldx FreqLoTable, y
		GotFreq:

			ldy SIDGhostbytes + (ChannelIndex * 7) + 6	//; sustain/release .. top 4 bits are sustain, bottom 4 bits are release
			lda ReleaseConversionHi, y
			sta MeterReleaseHiPerChannel + ChannelIndex
			lda ReleaseConversionLo, y
			sta MeterReleaseLoPerChannel + ChannelIndex

			lda SustainConversion, y
			cmp MeterTempHi + 0, x
			bcc !skipUpdate-

		DoUpdate:
			sta MeterTempHi + 0, x
			lda #0
			sta MeterTempLo + 0, x

		//; LHS neighbour bar
			lda MeterTempLo + 0, x
			clc
			adc MeterTempLo - 2, x
			sta !TempLo+ + 1

			lda MeterTempHi + 0, x
			adc MeterTempHi - 2, x
			sta !TempHi+ + 1

			lsr !TempHi+ + 1
			ror !TempLo+ + 1

		!TempHi:
			lda #$00
			cmp MeterTempHi - 1, x
			bcc NoUpdateL

			tay

		!TempLo:
			lda #$00
			cmp MeterTempLo - 1, x
			bcc NoUpdateL

			sta MeterTempLo - 1, x
			tya
			sta MeterTempHi - 1, x

		NoUpdateL:

		//; RHS neighbour bar
			lda MeterTempLo + 0, x
			clc
			adc MeterTempLo + 2, x
			sta !TempLo+ + 1

			lda MeterTempHi + 0, x
			adc MeterTempHi + 2, x
			sta !TempHi+ + 1

			lsr !TempHi+ + 1
			ror !TempLo+ + 1

		!TempHi:
			lda #$00
			cmp MeterTempHi + 1, x
			bcc NoUpdateR

			tay

		!TempLo:
			lda #$00
			cmp MeterTempLo + 1, x
			bcc NoUpdateR

			sta MeterTempLo + 1, x
			tya
			sta MeterTempHi + 1, x

		NoUpdateR:

		NoUpdate:
		}
		rts


//; MUSICPLAYER_DrawSpectrum() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_DrawSpectrum:

		.for (var i = 0; i < NUM_FREQS_ON_SCREEN; i++)
		{
			ldx dBMeterValue + i
			cpx LastFrame_bBMeterValue + i
			beq !noupdate+

		!doupdate:
			stx LastFrame_bBMeterValue + i

			.for (var Line = 0; Line < TopSpectrometerHeight; Line++)
			{
				lda MeterToCharValues - MeterToChar_PrePadding + (Line * 8), x
				sta ScreenAddress + ((SPECTROMETER_StartLine + Line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
			}

			lda Div3, x
			tay
			.for (var Line = 0; Line < BottomSpectrometerHeight; Line++)
			{
				lda MeterToCharValues - 17 + (Line * 8), y
				clc
				adc ReflectionFrameCharVal + 1
				sta ScreenAddress + ((SPECTROMETER_StartLine + TopSpectrometerHeight + BottomSpectrometerHeight - 1 - Line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
			}

		!noupdate:

			lda MeterColourValues, x
			cmp LastFrame_Colours + i
			beq !noupdate+
			sta LastFrame_Colours + i

			.for (var Line = 0; Line < TopSpectrometerHeight; Line++)
			{
				sta $d800 + ((SPECTROMETER_StartLine + Line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
			}

			lda MeterColourValues_Darker, x
			.for (var Line = 0; Line < BottomSpectrometerHeight; Line++)
			{
				sta $d800 + ((SPECTROMETER_StartLine + TopSpectrometerHeight + BottomSpectrometerHeight - 1 - Line) * 40) + ((40 - NUM_FREQS_ON_SCREEN) / 2) + i
			}

		!noupdate:
		}

	ReflectionFrameCharVal:
		lda #10
		sec
		sbc #10
		bne !good+
		lda #20
	!good:
		sta ReflectionFrameCharVal + 1

		rts

//; MUSICPLAYER_PlayMusic() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_PlayMusic:

		lda $01
		pha
		lda #$30
		sta $01

		jsr SIDPlay

		.for (var i=24; i >= 0; i--)
		{
			lda $d400 + i
			sta SIDGhostbytes + i
		}
		pla
		sta $01

		#if SID_REGISTER_REORDER_AVAILABLE
			.for (var i = 0; i < SIDRegisterCount; i++) {
				lda SIDGhostbytes + SIDRegisterOrder.get(i)
				sta $d400 + SIDRegisterOrder.get(i)
			}
		#else //; SID_REGISTER_REORDER_AVAILABLE
			.for (var i = 0; i <= 24; i++) {
				lda SIDGhostbytes + i
				sta $d400 + i
			}
		#endif

		jmp MUSICPLAYER_Spectrometer_PerPlay

//; MUSICPLAYER_SetupNewSong() -------------------------------------------------------------------------------------------------------
MUSICPLAYER_SetupNewSong:

		ldy #$00
		lda #$00
	BlankMusicLoop:
		sta $d400, y
		sta SIDGhostbytes, y
		iny
		cpy #25
		bne BlankMusicLoop

		ldy #SongNameLength - 1
	SongNameDisplayLoop:
		lda SongData_SongName, y
		sta ScreenAddress + ((SongNameScreenLine + 0) * 40) + SongNamePadding, y
		ora #$80
		sta ScreenAddress + ((SongNameScreenLine + 1) * 40) + SongNamePadding, y
		dey
		bpl SongNameDisplayLoop

		ldy #ArtistNameLength - 1
	ArtistNameDisplayLoop:
		lda SongData_ArtistName, y
		sta ScreenAddress + ((ArtistNameScreenLine + 0) * 40) + ArtistNamePadding, y
		ora #$80
		sta ScreenAddress + ((ArtistNameScreenLine + 1) * 40) + ArtistNamePadding, y
		dey
		bpl ArtistNameDisplayLoop

		ldx #NUM_FREQS_ON_SCREEN + 3
		lda #$00
	!loop:
		sta MeterChannel - 2, x
		sta MeterTempHi - 2, x
		sta MeterTempLo - 2, x
		dex
		bpl !loop-

		ldx #$02
		lda #$00
	!loop:
		sta MeterReleaseLoPerChannel, x
		sta MeterReleaseHiPerChannel, x
		dex
		bpl !loop-

		ldx #NUM_FREQS_ON_SCREEN - 1
		lda #$00
	!loop:
		sta dBMeterValue, x
		dex
		bpl !loop-

		lda #$00
		jmp SIDInit

//; PreDemoSetup() -------------------------------------------------------------------------------------------------------
PreDemoSetup:

		//; from Spindle by lft, www.linusakesson.net/software/spindle/
		//; Prepare CIA #1 timer B to compensate for interrupt jitter.
		//; Also initialise d01a and dc02.
		//; This code is inlined into prgloader, and also into the
		//; first effect driver by pefchain.
		bit	$d011
		bmi	*-3

		bit	$d011
		bpl	*-3

		ldx	$d012
		inx
	resync:
		cpx	$d012
		bne	*-3

		ldy	#0
		sty	$dc07
		lda	#62
		sta	$dc06
		iny
		sty	$d01a
		dey
		dey
		sty	$dc02
		cmp	(0,x)
		cmp	(0,x)
		cmp	(0,x)
		lda	#$11
		sta	$dc0f
		txa
		inx
		inx
		cmp	$d012
		bne	resync

		//; Regular IRQ setup
		lda #$7f
		sta $dc0d
		sta $dd0d
		lda $dc0d
		lda $dd0d

		bit $d011
		bpl *-3
		bit $d011
		bmi *-3

		lda #$01
		sta $d01a

		rts
//; ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

.import source "..\INC\NMIFix.asm"
.import source "..\INC\RasterLineTiming.asm"

EndCodeSegment:

* = ADDR_WaterSprites
StartWaterSprites:
	.fill watersprites_map.getSize(), watersprites_map.get(i)
EndWaterSprites:

* = ADDR_CharSet
StartCharSet:
	.fill charset_map.getSize(), charset_map.get(i)
EndCharSet:

	.print "* $" + toHexString(StartCodeSegment) + "-$" + toHexString(EndCodeSegment - 1) + " Code Segment"
	.print "* $" + toHexString(StartLocalData) + "-$" + toHexString(EndLocalData - 1) + " Local Data"
	.print "* $" + toHexString(StartCharSet) + "-$" + toHexString(EndCharSet - 1) + " Char Set"
	.print "* $" + toHexString(StartWaterSprites) + "-$" + toHexString(EndWaterSprites - 1) + " Water Sprites"
