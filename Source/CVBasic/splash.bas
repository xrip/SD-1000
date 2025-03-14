	'
	' Test for multiplication/division/modulo/READ/DATA (for CVBasic)
	'
	' by Oscar Toledo G.
	' https://nanochess.org/
	'
	' Creation date: Feb/29/2024.
	'
    ASM LD A,$92
	ASM OUT ($DF),A
	ASM LD A,$07
	ASM OUT ($DE),A
	

	MODE 1
	SCREEN DISABLE
	DEFINE VRAM PLETTER $0000,$1800,image_bitmap
	DEFINE VRAM PLETTER $2000,$1800,image_color
	SCREEN ENABLE

  
loop:
	GOSUB reset_sound
    for A=1 to 2 
        IF A=1 THEN #C=477
        IF A=2 THEN #C=239
    SOUND 0,#C,11
    SOUND 1,(#C+1)/2,5
    SOUND 2,#C*2,8
    SOUND 3,6000,10 ' Slow decay, single shot \______
    FOR #D = 1 TO 30
	 wait 
	NEXT #D
    NEXT A
	for #D=1 to 10
	wait
	NEXT #D
   
   GOSUB reset_sound
   for i=0 to 90
     WAIT
   Next i   
   
   
   mode 0
   ' Make a reverse font
	FOR #c = $000 TO $3ff
		d = VPEEK(#c)
		d = d XOR $ff 
		VPOKE #c+$400,d 
	NEXT #c
    FOR #c = $800 TO $cff
		d = VPEEK(#c)
		d = d XOR $ff
		VPOKE #c+$400,d 
	NEXT #c
	   FOR #c = $1000 TO $13ff
		d = VPEEK(#c)
		d = d XOR $ff
		VPOKE #c+$400,d 
	NEXT #c
	
inizio:
   mode 0
   cls
   BORDER 4
   
   'poke 16128,1 ' read dir
   poke 30000,1 ' read dir
   DEFINE CHAR 128,8,riga
   DEFINE COLOR 128,8,riga_color
   
newdir:
   cls
   BORDER 4
   print at 34+6,"SEGA SD-1000 v.1.0"
  
   VPOKE $1800,130
   FOR c= 1 TO 30
     VPOKE $1800+c,128
   NEXT c 
   VPOKE $1800+31,131
   VPOKE $1800+32,129
   VPOKE $1800+63,129
   VPOKE $1800+64,135
   
   FOR c= 1 TO 30
     VPOKE $1800+64+c,128
   NEXT c 
   VPOKE $1800+95,134
   
    totfile=0
  	FOR c = 3 TO 22
      for i = 1 to 30
	  vpoke $1800+32*c,129
		vpoke $1800+32*c+i, peek(30002+((c-3)*32)+i-1)
	  next i 
	  if peek(30002+(32*(c-3)))>0 then totfile=totfile+1
	  vpoke $1800+32*c+31,129
	  if peek(31000+(c-3))=1 then 
	  end if
	  WAIT
	NEXT c
	
   VPOKE $1800+32*23,132
   FOR c= 1 TO 30
     VPOKE $1800+32*23+c,128
   NEXT c 
   VPOKE $1800+32*23+31,133
 
   numfile=1
   numfileold=0
 scegli:
   for c=1 to 10
     WAIT
   next c 
   
   for i=1 to 30
      d#=vpeek($1800+i+((numfile+2)*32))
      vpoke $1800+i+((numfile+2)*32),d#+128
	'  vpoke($1800+32*c+1),65+numfile
   NEXT i	  
    for i=1 to 30
      d#=vpeek($1800+i+((numfileold+2)*32))
      if numfileold<>0 then vpoke $1800+i+((numfileold+2)*32),d#-128
   NEXT i	 
   print at 32*23+2,"Btn1->Sel"
   print at 32*23+12,"Btn2->Dir-Up"

  ' nf=peek(51030)
  ' print at 32*23+1,"ROMS from:"
  ' print at 32*23+11,nf
  ' nf=peek(51031)
  ' print at 32*23+15,"to:"
  ' print at 32*23+18,nf
  ' nf=peek(51032)
  ' print at 32*23+22,"of:"
  ' print at 32*23+25,nf
   
  c=cont1
  while c=0
	wait 
	c=cont1  
	'print at 32*23+20,c 
 WEND
  sound 0,120,5
  WAIT
  sound 0,0,0 
  
   if (c and 128) then goto updir
   if (c and 64) then goto rungame
   if (c and 8)  THEN goto prevpage
   if (c and 4) and (numfile<totfile) THEN numfileold=numfile:numfile=numfile+1:goto scegli
   if (c and 2)  THEN goto nextpage
   if (c=1) and (numfile>1) THEN numfileold=numfile:numfile=numfile-1:goto scegli

   c=0
   goto scegli
    
nextpage:
   poke 29999,0 ' wait for goto
   poke 30000,3 ' next page
   print at 32*23,"--> Next DIR"
   while peek(29999)<>1:wend
 
   goto newdir
   
prevpage:
   poke 29999,0 ' wait for goto
   poke 30000,4 ' next page
   print at 32*23,"--> Prev DIR"
   while peek(29999)<>1:wend
   goto newdir
  
updir:
   poke 29999,0 ' wait for goto
   poke 30000,5 ' up Dir
   print at 32*23,"--> Up DIR"
   while peek(29999)<>1:wend
   goto newdir

rungame:  
  CLS  
  poke 30001,numfile ' file to run
  poke 30000,2 ' run file
  if peek(31000+numfile-1)=1 then  ' is opendir
  print at 32*23,"--> Open DIR"
  while peek(29999)<>1:wend
  goto newdir
  end IF
  ' else is run game
  
  print at 32*4,"Loading ROM:"
  for i=0 to 255
	vpoke $1800+32*5+i, peek(30002+i)
  next i
  for i=1 to 32
  border i
  wait
  next i
  
   for i=0 to 5
	 border i
	 wait
   next i
 
    cls
   

   poke $00,$f3 ' di
   poke $01,$f3 ' di
   poke $02,$f3 ' di 
   poke $03,$31:poke $04,0:poke $05,$0 'ld sp,c200h
   poke $06,$c3:poke $07,0:poke $08,0 ' jp 0
   poke $38,$f3 ' di
   poke $39,$f3 ' di
   poke $3a,$f3 ' di 
   poke $3b,$31:poke $3c,0:poke $3d,$0 'ld sp,c200h
   poke $3e,$c3:poke $3f,0:poke $40,0 ' jp 0
   WHILE 1:WEND
   
reset_sound:    PROCEDURE
      SOUND 0,,0
      SOUND 1,,0
      SOUND 2,,0
      SOUND 3,,0
    RETURN
    END

saved_string:
	DATA BYTE "Test string",0

    include "logo.bas"

riga:
DATA BYTE $00,$00,$00,$ff,$00,$00,$00,$00
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."

BITMAP "........"
BITMAP "........"
BITMAP "........"
BITMAP "....XXXX"
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."

BITMAP "........"
BITMAP "........"
BITMAP "........"
BITMAP "XXXXX..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."

BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....XXXX"
BITMAP "........"
BITMAP "........"
BITMAP "........"
BITMAP "........"

BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "XXXXX..."
BITMAP "........"
BITMAP "........"
BITMAP "........"
BITMAP "........"

BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "XXXXX..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."

BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....XXXX"
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."
BITMAP "....X..."

riga_color:
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4
DATA BYTE $a4,$a4,$a4,$a4,$a4,$a4,$a4,$a4

