!
! x^16 + x^12 + x^5 + 1 (aka xmodem, CRC-16-CCITT, etc)
!
!

	case on
	mcopy crc.mac


dummy	start
	end

table	privdata
	dc i2'$0000, $1021, $2042, $3063, $4084, $50a5, $60c6, $70e7'
	dc i2'$8108, $9129, $a14a, $b16b, $c18c, $d1ad, $e1ce, $f1ef'
	dc i2'$1231, $0210, $3273, $2252, $52b5, $4294, $72f7, $62d6'
	dc i2'$9339, $8318, $b37b, $a35a, $d3bd, $c39c, $f3ff, $e3de'
	dc i2'$2462, $3443, $0420, $1401, $64e6, $74c7, $44a4, $5485'
	dc i2'$a56a, $b54b, $8528, $9509, $e5ee, $f5cf, $c5ac, $d58d'
	dc i2'$3653, $2672, $1611, $0630, $76d7, $66f6, $5695, $46b4'
	dc i2'$b75b, $a77a, $9719, $8738, $f7df, $e7fe, $d79d, $c7bc'
	dc i2'$48c4, $58e5, $6886, $78a7, $0840, $1861, $2802, $3823'
	dc i2'$c9cc, $d9ed, $e98e, $f9af, $8948, $9969, $a90a, $b92b'
	dc i2'$5af5, $4ad4, $7ab7, $6a96, $1a71, $0a50, $3a33, $2a12'
	dc i2'$dbfd, $cbdc, $fbbf, $eb9e, $9b79, $8b58, $bb3b, $ab1a'
	dc i2'$6ca6, $7c87, $4ce4, $5cc5, $2c22, $3c03, $0c60, $1c41'
	dc i2'$edae, $fd8f, $cdec, $ddcd, $ad2a, $bd0b, $8d68, $9d49'
	dc i2'$7e97, $6eb6, $5ed5, $4ef4, $3e13, $2e32, $1e51, $0e70'
	dc i2'$ff9f, $efbe, $dfdd, $cffc, $bf1b, $af3a, $9f59, $8f78'
	dc i2'$9188, $81a9, $b1ca, $a1eb, $d10c, $c12d, $f14e, $e16f'
	dc i2'$1080, $00a1, $30c2, $20e3, $5004, $4025, $7046, $6067'
	dc i2'$83b9, $9398, $a3fb, $b3da, $c33d, $d31c, $e37f, $f35e'
	dc i2'$02b1, $1290, $22f3, $32d2, $4235, $5214, $6277, $7256'
	dc i2'$b5ea, $a5cb, $95a8, $8589, $f56e, $e54f, $d52c, $c50d'
	dc i2'$34e2, $24c3, $14a0, $0481, $7466, $6447, $5424, $4405'
	dc i2'$a7db, $b7fa, $8799, $97b8, $e75f, $f77e, $c71d, $d73c'
	dc i2'$26d3, $36f2, $0691, $16b0, $6657, $7676, $4615, $5634'
	dc i2'$d94c, $c96d, $f90e, $e92f, $99c8, $89e9, $b98a, $a9ab'
	dc i2'$5844, $4865, $7806, $6827, $18c0, $08e1, $3882, $28a3'
	dc i2'$cb7d, $db5c, $eb3f, $fb1e, $8bf9, $9bd8, $abbb, $bb9a'
	dc i2'$4a75, $5a54, $6a37, $7a16, $0af1, $1ad0, $2ab3, $3a92'
	dc i2'$fd2e, $ed0f, $dd6c, $cd4d, $bdaa, $ad8b, $9de8, $8dc9'
	dc i2'$7c26, $6c07, $5c64, $4c45, $3ca2, $2c83, $1ce0, $0cc1'
	dc i2'$ef1f, $ff3e, $cf5d, $df7c, $af9b, $bfba, $8fd9, $9ff8'
	dc i2'$6e17, $7e36, $4e55, $5e74, $2e93, $3eb2, $0ed1, $1ef0'
	end


!
! crc update(crc, data, len)
!
crc_update	start

	using table

	csub (2:crc,4:ptr,2:len)

	lda len
	beq exit

	ldy #0
	lda crc
loop	anop
! x = ((crc >> 8) ^ c ) & 0xff
	xba
	and #$ff
	eor [ptr],y
	and #$ff
	asl a
	tax
! crc = table[x] ^ (crc << 8)
	lda crc
	xba
	and #$ff00
	eor table,x
	sta crc
	iny
	dec len
	bne loop

exit	anop
	ret 2:crc
	end
