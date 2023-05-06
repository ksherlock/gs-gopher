!
! Link control.
! This is a replacement for a static text control
! since fSquishText doesn't support colors and doesn't support
! resources or handles. (as of 6.0.1)
!
!

	mcopy linkctrl.mac
	copy 13:AInclude:E16.Control
	copy 13:AInclude:E16.Resources
	mcopy M16.SmartStacks

	case on
!	list on
!	gen on

dummy	start
	end



linkctrl	start
! Inputs:           On entry, the parameters are passed to us on the stack.
!
!                   |                   | Previous Contents
!                   |-------------------|
!                   |    ReturnValue    | LONG - Space for return value
!                   |-------------------|
!                   |    CtlCode        | WORD - operation to perform
!                   |-------------------|
!                   |    CtlParam       | LONG - add'l parameter
!                   |-------------------|
!                   |    theCtlHandle   | LONG - Handle to control record
!                   |-------------------|
!                   |    RtnAddr        | 3 BYTES - RTL address
!                   |-------------------|
!                   |                   | <-- Stack pointer
!
! Outputs:          Put something into ReturnValue, pull off the parameters,
!                   and return to the Control Manager.
!
!                   |                   | Previous Contents
!                   |-------------------|
!                   |    ReturnValue    | LONG - Space for return value
!                   |-------------------|
!                   |    RtnAddr        | 3 BYTES - RTL address
!                   |-------------------|
!                   |                   | <-- Stack pointer



; lock resource
	pha
	pha
	~FindHandle #linkctrl
	_HLock


	DefineStack
&gequ	setb  1 ; dirty hack to make these global.
ptr	longword
control longword
	EndLocals
	FixStack
	BegParms
handle	longword
param	longword
code	word
	EndParms
rv	word
	BeginStack


	stz <rv
	stz <rv+2

	lda <code
	cmp #(TableEnd-TableStart)/2
	bcs exit

	asl a
	tax
	lda TableStart,x
	beq exit
	lda [handle]
	sta control
	ldy #2
	lda [handle],y
	sta control+2

! x still valid.
	jsr (TableStart,x)
	stax <rv

exit	anop

; unlock resource
	pha
	pha
	~FindHandle #linkctrl
	_HUnlock


	EndStack
	rtl


rts	anop
	lda #0
	ldx #0


TableStart anop
	dc a2'link_draw'	; draw control
	dc a2'0'	; calc control rect
	dc a2'link_test'	; test control
	dc a2'link_init'	; init control
	dc a2'0'	; dispose control
	dc a2'0'	; position control
	dc a2'0'	; thumb
	dc a2'0'	; drag
	dc a2'0'	; auto track
	dc a2'link_draw'	; new value
	dc a2'0'	; set params
	dc a2'0'	; move control
	dc a2'link_recsize'	; record size

TableEnd anop
	end

data	privdata

buffer	ds 256

	end

link_draw	private


	using data

!	brk $ea


! find the data type...

	ldy #octlData
	lda [control],y
	sta ptr
	iny
	iny
	lda [control],y
	sta ptr+2


	ldy #octlMoreFlags
	lda [control],y
	and #$03
	asl a
	tax
	jsr (table,x)


! ptr = 0
! Handle = 1
! resource = 2



	pha
	_GetForeColor

! 1111 = dark blue
! 4444 = dark red 

	_PenNormal
	~SetFontFlags #4 ; very important.
	~SetForeColor #$1111
	~SetBackColor #$ffff


! erase the old rectangle...
	lda #octlRect
	clc
	adc control
	tax
	lda #0
	adc control+2
	pha
	phx
	_EraseRect


! left edge
	ldy #octlRect+2
	lda [control],y
	pha 
! top
	dey
	dey
	lda [control],y
	clc
	adc #8
	pha
	_MoveTo

! right truncation, p-string
	ph2 #%0110000000000000
	ph4 #buffer

! width
	ldy #octlRect+6
	lda [control],y
	sec
	ldy #octlRect+2
	sbc [control],y
	pha

	_DrawStringWidth

! restore
	_SetForeColor

	ldax #0
	rts

table	anop
	dc a2'pointer'
	dc a2'handle'
	dc a2'resource'
	dc a2'invalid'

invalid	anop
	stz buffer
	rts

pointer	anop

	ph4 <ptr
	ph4 #buffer+1

	ldy #octlValue
	lda [control],y
	and #$00ff
	sta buffer

	pea 0
	pha
	_BlockMove
	clc
	rts


resource anop
	pha
	pha
	~LoadResource #rTextForLETextBox2,<ptr
	pla
	sta ptr
	pla
	sta ptr+2
	bcc rok
	stz buffer
	rts
rok	anop

handle	anop

	~HLock <ptr

	ldy #2
	lda [ptr],y
	pha
	lda [ptr]
	pha

	ph4 #buffer+1


	ph4 #0
	~GetHandleSize <ptr
	pla
	plx
	and #$00ff
	sta buffer
	pea 0
	pha
	_BlockMove
	clc
	rts

	end

link_test	private

	using data

	ldy #octlFlag ; and hilite
	lda [control],y
	bit #ctlInVis
	bne nope
	and #$ff00
	cmp #$ff00
	beq nope

	ldax #1
	rts


nope	anop
	ldax #0
	rts

	end

link_init	private

! justification, size (if this is a ptr) not currently supported.

! copy the data.
oTemplateTextRef equ $1a
! this just happens to be 2 bytes before the location in the control record
	ldy #oTemplateTextRef
	lda [param],y
	iny
	iny
	sta [control],y
	lda [param],y
	iny
	iny
	sta [control],y

	ldax #0

	end

link_recsize	start
	ldax #$40
	rts
	end
