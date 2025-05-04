.var FrameHeight = IsPAL ? 312 : 262

D012_Values:
.for (var i=0; i<NumCallsPerFrame; i++) {
    .var line = FrameHeight * (i + 0.5) / NumCallsPerFrame
    .byte line & $ff
}

D011_Values:
.for (var i=0; i<NumCallsPerFrame; i++) {
    .var line = FrameHeight * (i + 0.5) / NumCallsPerFrame
    .byte (line >= 256) ? $80 : $00
}
