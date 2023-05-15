!
! StringControl
! This is similar to a Static Text Control; but
! 1. uses a pString/rPString instead of rLETextBox2
! 2. therefore doesn't need to set the value to the text length
! 3. uses a color table for foreground/background
! (instead of rLETextBox inline codes)
! 4. uses DrawStringWidth() to draw
!
! (So, static text with fBlast + fSquish, but more control over colors and drawing)
! (and a more human API for changing the text)
!
! load the string into ptr
!

	copy 13:AInclude:E16.Control
	copy 13:AInclude:E16.Resources
	mcopy M16.SmartStacks
	mcopy stringctrl.mac


	case on


dummy	private
	end


records	privdata
! control fields
	Record $28
oDrawStringFlags word

!	EndRecord

	end


control_entry	private



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

; lock resource
!	pha
!	pha
!	~FindHandle #control_entry
!	_HLock


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
!	pha
!	pha
!	~FindHandle #control_entry
!	_HUnlock


	EndStack
	rtl


rts	anop
	lda #0
	ldx #0


TableStart anop
	dc a2'control_draw'	; draw control
	dc a2'0'	; calc control rect
	dc a2'control_test'	; test control
	dc a2'control_init'	; init control
	dc a2'control_dispose'	; dispose control
	dc a2'0'	; position control
	dc a2'0'	; thumb
	dc a2'0'	; drag
	dc a2'0'	; auto track
	dc a2'control_draw'	; new value
	dc a2'0'	; set params
	dc a2'0'	; move control
	dc a2'control_size'	; record size

TableEnd anop
	end

	end


control_init private

!
! ID, rectangle, flag, moreflag, refcon, proc, are already set by NewControl / NewControl2.
! All other fields are zero.
!

	using records

	DSect
tPCount	word
tID	longword
tRect	block 8
tProc	longword
tFlag	word ;4 
tMoreFlags word ; 5
tRefCon	longword ; 6
tTextRef	longword ; 7
tDrawStringFlags word ; 8
tColorRef longword ; 9
tValue	word ; 10 ; 0 / 1 to choose color
!tSize	EndDSect

! optional fields
	lda [param]
	cmp #11
	bcc ok
	lda #10
ok	asl a
	tax
	jmp (table,x)
table	dc a2'p0,p1,p2,p3,p4'
	dc a2'p5,p6,p7,p8,p9'
	dc a2'p10'

p10	anop ; value
	ldy #tValue
	lda [param],y
	ldy #octlValue
	sta [control],y

p9	anop ; color ref
	ldy #tColorRef
	lda [param],y
	tax
	iny
	iny
	lda [param],y

	ldy #octlColor+2
	sta [control],y
	dey
	dey
	txa
	sta [control],y

p8	anop ; draw string flags
	ldy #tDrawStringFlags
	lda [param],y
	ldy #oDrawStringFlags
	and #%1111111111111100  ; must be p-string.
	sta [control],y

p7	anop
	ldy #tTextRef
	lda [param],y
	tax
	iny
	iny
	lda [param],y

	ldy #octlData+2
	sta [control],y
	dey
	dey
	txa
	sta [control],y

p6	anop
p5	anop
p4	anop
p3	anop
p2	anop
p1	anop
p0	anop

	rts
	end


control_size	private
	ldax #$40
	rts
	end

control_test	private
! return the part code
! control manager has already checked that we are enabled
! and visible and that the point is within the rect.
	ldax #1
	rts
	end

control_dispose	private
!
! if the string or color table are resources, release them.
!

ct	anop

	ldy #octlColor
	lda [control],y
	sta ptr
	iny
	iny
	lda [control],y
	sta ptr+2
	ora ptr
	beq str

	ldy #octlMoreFlags
	lda [control],y
	and #%1100
	cmp #%1000
	bne str

	~ReleaseResource #3,#rCtlColorTbl,<ptr

str	anop

	ldy #octlData
	lda [control],y
	sta ptr
	iny
	iny
	lda [control],y
	sta ptr+2
	ora ptr
	beq rts

	ldy #octlMoreFlags
	lda [control],y
	and #%0011
	cmp #%0010
	bne rts

	~ReleaseResource #3,#rPString,<ptr

rts	rts
	end




control_draw	private
!
! Draw the control. (Also called for new value)
!

	using records

	pha
	_GetFontFlags
	pha
	_GetForeColor
	pha
	_GetBackColor

! set up the colors, etc.
	_PenNormal
	~SetFontFlags #4 ; 16 dithered colors.

	jsr load_ct
	lda ptr
	ora ptr+2
	beq def_ct

	ldy #octlValue
	lda [control],y
	and #1
	asl a
	asl a
	tay
	lda [ptr],y
	pha
	iny
	iny
	lda [ptr],y
	pha
	_SetBackColor
	_SetForeColor
	bra erase

def_ct	anop

	~SetForeColor #$0000
	~SetBackColor #$ffff

erase	anop
! erase the previous data
	lda #octlRect
	clc
	adc control
	tax
	lda #0
	adc control+2
	pha
	phx
	_EraseRect

str	anop
! draw the string

	jsr load_string
	lda ptr
	ora ptr+2
	beq exit

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

	ldy #oDrawStringFlags
	lda [control],y
	pha
	ph4 <ptr

	ldy #octlRect+6
	lda [control],y
	sec
	ldy #octlRect+2
	sbc [control],y
	pha

	_DrawStringWidth

exit	anop

!	~SetBackColor #$ffff
!	~SetForeColor #$0000
	_SetBackColor
	_SetForeColor
	_SetFontFlags

	rts

	end

!
! load the string into ptr.
!
load_string	private

	ldy #octlData
	lda [control],y
	sta ptr
	iny
	iny
	lda [control],y
	sta ptr+2

	ldy #octlMoreFlags
	lda [control],y
	and #%0011
	asl a
	tax
	jmp (table,x)


table	anop
	dc a2'is_ptr'
	dc a2'is_handle'
	dc a2'is_resource'
	dc a2'is_invalid'

is_invalid anop
	stz ptr
	stz ptr+2

is_ptr anop
	rts

is_resource anop
	pha
	pha
	~LoadResource #rPString,<ptr
	plax
	bcs is_invalid
	stax <ptr

is_handle	anop

	~HLock <ptr
	ldy #2
	lda [ptr],y
	tax
	lda [ptr]
	sta ptr
	stx ptr+2
	rts

	end


!
! load the color table into ptr.
!
load_ct private

	ldy #octlColor
	lda [control],y
	sta ptr
	iny
	iny
	lda [control],y
	sta ptr+2

	ldy #octlMoreFlags
	lda [control],y
	and #%1100
	lsr a
	tax
	jmp (table,x)


table	anop
	dc a2'is_ptr'
	dc a2'is_handle'
	dc a2'is_resource'
	dc a2'is_invalid'

is_invalid anop
	stz ptr
	stz ptr+2

is_ptr anop
	rts

is_resource anop
	pha
	pha
	~LoadResource #rCtlColorTbl,<ptr
	plax
	bcs is_invalid
	stax <ptr

is_handle	anop

	~HLock <ptr
	ldy #2
	lda [ptr],y
	tax
	lda [ptr]
	sta ptr
	stx ptr+2
	rts

	end
