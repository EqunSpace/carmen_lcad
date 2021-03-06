CON
  _clkmode = xtal1 + pll16x
  _xinfreq = 5_000_000

  LCD_PIN = 7
  DRAW = 1
  
DAT

OBJ
        GraphicLcdSerial:"FullDuplexSerial"
        'AtariFont        :"Font_ATARI"

VAR
        long stack[64]
PUB Start(TxPin) : success | cog
        'Start the sabertooth runtime in a new cog
        success := (cog := cognew(GraphicLcdRuntime(TxPin), @stack) +1)
       
PUB GraphicLcdRuntime(TxPin)|I
        'Initialize the serial port
        GraphicLcdSerial.start(-1,TxPin,0,115200)

        waitcnt(clkfreq/40+cnt)
        'ClearScreen     
        
                                

PUB dec(value)
  GraphicLcdSerial.dec(value)         
PUB Grid(OnOff)|I
         repeat I from 0 to 15
          DrawLine(I*10,0,I*10,127,OnOff)
          waitcnt(clkfreq/40+cnt) 
         repeat I from 0 to 12
          DrawLine(0,I*10,159,I*10,OnOff)
          waitcnt(clkfreq/40+cnt)            
PUB ClearScreen
{
Sending <control>@ (0x00)"clears the screen of all
written pixels. If you're operating in normal mode, all
pixels are reset. If you're operating in reverse mode,
all pixels are set (white background). An example of a
clear screen" command would be to send 0x7C 0x00,
or from a keyboard send |"and <control>@.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($00)
'  waitcnt(clkfreq/40+cnt)  

PUB Demo
{
Sending <control>d (0x04)"runs demonstration
code. This is in the firmware just as an example of
what the display can do. To see the demonstration,
send 0x7C 0x04, or from a keyboard send |"and
<control>d.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($04)

PUB ReverseMode
{
Sending <control>r (0x18)"toggles between white on
blue display and blue on white display. Setting the
reverse mode causes the screen to immediately clear
with the new background. To set the reverse mode,
send 0x7C 0x18, or from a keyboard send |"and
<control>r. This setting is saved between power
cycles. If the display is turned off while in reverse
mode, it will next power up in reverse mode.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(18)

PUB SplashScreen
{
Sending <control>s (0x13)"allows or disallows the
SparkFun logo to be displayed at power up. The
splash screen serves two purposes. One is obviously
to put our mark on the product, but the second is to
allow a short time at power up where the display can
be recovered from errant baud rate changes (see
Baud Rate for more info). Disabling the splash screen
suppresses the logo, but the delay remains active. To
disable the splash screen, send 0x7C, 0x13, or from
a keyboard send |"and <control>s.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(19)
  
PUB SetBacklight(value)
{
Sending <control>b (0x02)"followed by a number
from 0 to 100 whill change the backlight intensity (and
therefore current draw). Setting the value to zero
turns the back light off, setting it at 100 or above turns
it full on, and intermediate values set it somewhere inbetween.
The number setting in the command
sequence is an 8-bit ASCII value. As an example, to
set the backlight duty cycle to 50, send 0x7C 0x02
0x32, or from a keyboard send |" <control>b and   ."
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($02)
  GraphicLcdSerial.tx(value)  

PUB MoveXY(x,y)
{
Sending <control>x ( 0x17)"or  control>y (0x18)  "
followed by a number representing a new reference
coordinate changes the X or Y coordinates. The X
and Y reference coordinates (x_offset and y_offset in
the source code) are used by the text generator to
place text at specific locations on the screen. As
stated earlier, the coordinates refer to the upper left
most pixel in the character space. If the offsets are
within 6 pixels of the right edge of the screen or 8
pixels of the bottom, the text generator will revert to
the next logical line for text so as to print a whole
character and not parts. As an example, to set
x_offset to 80 (the middle of the horizontal axis) send
0x7C 0x17 0x50, or from a keyboard send |"
<control>x and P" Attempting to set values greater
than the length of each axis
}
 MoveX(x)
 MoveY(y)
PUB MoveX(x)
  GraphicLcdSerial.tx($7C)  
  GraphicLcdSerial.tx(24)
  GraphicLcdSerial.tx(x)
'  waitcnt(clkfreq/40+cnt)
PUB MoveY(y)
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(25)
  GraphicLcdSerial.tx(y)
  'waitcnt(clkfreq/40+cnt)
  
PUB SetPixel(x,y,drawOrErase)
{
Sending <control>p (0x10)"followed by x and y
coordinates, and a 0 or 1 to determine setting or
resetting of the pixel. Any pixel on the display can
independently set or reset with this command. As an
example, to set the pixel at (80, 64) send 0x7C 0x10
0x50 0x40 0x01, or from a keyboard send |"
<control>p, P"    a"d <control>a. Remember that
setting a pixel doesn't necessarily mean writing a one
to that location, it means to write the opposite of the
background. So if you're operating in reverse mode,
setting a pixel actually clears the pixel and sets it
apart from the white background. Resetting that pixel
causes it to be white like the background.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(16)
  GraphicLcdSerial.tx(x)
  GraphicLcdSerial.tx(y)
  GraphicLcdSerial.tx(drawOrErase)
  'waitcnt(clkfreq/40+cnt)  
     
PUB DrawLine(x1,y1,x2,y2,drawOrErase)
{
Sending <control>l (0x0C)"followed by two sets of
(x, y) coordinates defining the line's start and stop,
followed by a 0 or 1 determines whether to draw or
erase the line. As an example, to draw a line from
(0,10) to (50,60) send 0x7C 0x0C 0x00 (x1) 0x0A (y1)
0x32 (x2) 0x3C (y2) 0x01, or from a keyboard send
|", <control>l, <control>@, <control>j, 2"    a"d
<control>a. To erase the line (and leave surrounding
text and graphics unchanged), submit the same
command but changing the last <control>a to
<control>@.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($0C)
  GraphicLcdSerial.tx(x1)
  GraphicLcdSerial.tx(y1)
  GraphicLcdSerial.tx(x2)
  GraphicLcdSerial.tx(y2)  
  GraphicLcdSerial.tx(drawOrErase)
  'waitcnt(clkfreq/40+cnt)  
  
PUB DrawCircle(x,y,radius,drawOrErase)
{
Sending "<control>c (0x03)"followed by x and y
coordinates defining the center of the circle, followed
by a number representing the radius of the circle,
followed by a 0 or 1 determines whether to draw or
erase the circle. As an example, to draw a circle at
center (80, 64) with radius 10 send 0x7C 0x03 0x50
0x40 0x0A 0x01, or from a keyboard send |"
<control>c, "P" "@",<control>j and <control>a. To
erase the circle (and leave surrounding text and
graphics unchanged), submit the same command but
changing the last <control>a to <control>@. Circles
can be drawn off-grid, but only those pixels that fall
within the display boundaries will be written.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($03)
  GraphicLcdSerial.tx(x)
  GraphicLcdSerial.tx(y)
  GraphicLcdSerial.tx(radius)  
  GraphicLcdSerial.tx(drawOrErase)
  'waitcnt(clkfreq/40+cnt)  
  
PUB DrawBox(x1,y1,x2,y2,drawOrErase)
{
Sending "<control>o (0x0F)"followed by two sets of
(x, y) coordinates defining opposite corners of the
box, followed by a 0 or 1 determines whether to draw
or erase the box. This command is exactly like the
draw line command, but instead of drawing a line you
get a box that exactly contains the line between the
given coordinates. As an example, to draw a
rectangular box around the line from (0,10) to (50,60)
send 0x7C 0x0F 0x00 (x1) 0x0A (y1) 0x32 (x2) 0x3C
(y2) 0x01, or from a keyboard send "|", <control>o,
<control>@, <control>j, "2","<" and <control>a. To
erase the box (and leave surrounding text and
graphics unchanged), submit the same command but
changing the last <control>a to <control>@.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($0F)
  GraphicLcdSerial.tx(x1)
  GraphicLcdSerial.tx(y1)
  GraphicLcdSerial.tx(x2)
  GraphicLcdSerial.tx(y2)  
  GraphicLcdSerial.tx(drawOrErase)
  'waitcnt(clkfreq/40+cnt)

PUB DrawSolidBox(x1,y1,x2,y2,drawOrErase)
{
Sending "<control>o (0x0F)"followed by two sets of
(x, y) coordinates defining opposite corners of the
box, followed by a 0 or 1 determines whether to draw
or erase the box. This command is exactly like the
draw line command, but instead of drawing a line you
get a box that exactly contains the line between the
given coordinates. As an example, to draw a
rectangular box around the line from (0,10) to (50,60)
send 0x7C 0x0F 0x00 (x1) 0x0A (y1) 0x32 (x2) 0x3C
(y2) 0x01, or from a keyboard send "|", <control>o,
<control>@, <control>j, "2","<" and <control>a. To
erase the box (and leave surrounding text and
graphics unchanged), submit the same command but
changing the last <control>a to <control>@.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(14)
  GraphicLcdSerial.tx(x1)
  GraphicLcdSerial.tx(y1)
  GraphicLcdSerial.tx(x2)
  GraphicLcdSerial.tx(y2)  
  GraphicLcdSerial.tx(drawOrErase)
  'waitcnt(clkfreq/40+cnt)

PUB bar(x1,y1,x2,y2,value,v_h)
{
Sending "<control>o (0x0F)"followed by two sets of
(x, y) coordinates defining opposite corners of the
box, followed by a 0 or 1 determines whether to draw
or erase the box. This command is exactly like the
draw line command, but instead of drawing a line you
get a box that exactly contains the line between the
given coordinates. As an example, to draw a
rectangular box around the line from (0,10) to (50,60)
send 0x7C 0x0F 0x00 (x1) 0x0A (y1) 0x32 (x2) 0x3C
(y2) 0x01, or from a keyboard send "|", <control>o,
<control>@, <control>j, "2","<" and <control>a. To
erase the box (and leave surrounding text and
graphics unchanged), submit the same command but
changing the last <control>a to <control>@.
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(8)
  GraphicLcdSerial.tx(x1)
  GraphicLcdSerial.tx(y1)
  GraphicLcdSerial.tx(x2)
  GraphicLcdSerial.tx(y2)
  GraphicLcdSerial.tx(value)  
  GraphicLcdSerial.tx(v_h)
  waitcnt(clkfreq/20+cnt)
'  GraphicLcdSerial.tx(($7C+8+x1+y1+x2+y2+value+v_h)& $FF )       

  'waitcnt(clkfreq/40+cnt)    
PUB EraseBlock(x1,y1,x2,y2)
{
Sending "<control>e (0x05)"followed by two sets of
(x, y) coordinates defines opposite corners of the
block to be erased. This is just like the draw box
command, except the contents of the box are erased
to the background color. As an example, to erase a
rectangular block around the line from (0,10) to
(50,60) send 0x7C 0x05 0x00 (x1) 0x0A (y1) 0x32
(x2) 0x3C (y2)", or from a keyboard send |"
<control>e, <control>@, <control>j, 2"and "<".
}
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx($05)
  GraphicLcdSerial.tx(x1)
  GraphicLcdSerial.tx(y1)
  GraphicLcdSerial.tx(x2)
  GraphicLcdSerial.tx(y2)
  'waitcnt(clkfreq/40+cnt)  

PUB ChangeBaudRate(baud) |baudcode
{
Sending <control>g (0x07)"followed by an ASCII
character from"1" to"6"canges the baud rate. The
default baud rate is 115,200bps, but the backpack
can be set to a variety of communication speeds:
         "1" = 4800bps
         "2" = 9600bps
         "3" = 19,200bps
         "4" = 38,400bps
         "5" = 57,600bps
         "6" = 115,200bps
As an example, to set the baud rate to 19,200bps,
send 0x7C 0x07 0x33, or from a keyboard send"|"
<control>g and"3" The baud rate setting is retained
during power cycling, so if it powers down at
19,200bps, it will next power up with that setting.
In a pinch, the baud rate can be reset to 115,200.
During the one second delay at power up, send the
display any character at 115,200bps. The number
115200" will be shown in the upper left corner of the
display, and the backpack will revert to that setting
until otherwise changed.
}

  if(baud == 4800)
    baudcode := 1
  elseif(baud == 9600)  
    baudcode := 2
  elseif(baud == 19200)
    baudcode := 3
  elseif(baud == 38400)
    baudcode := 4
  elseif(baud == 57600)
    baudcode := 5
  elseif(baud == 115200)
    baudcode := 6
  else
    baudcode := 6  
          
  GraphicLcdSerial.tx($7C)
  waitcnt(clkfreq/40+cnt)
  GraphicLcdSerial.tx(7)
  waitcnt(clkfreq/40+cnt)
  GraphicLcdSerial.tx(baudcode)
  waitcnt(clkfreq/40+cnt)

PUB str(strAddr)
  repeat strsize(strAddr)
    GraphicLcdSerial.tx(byte[strAddr++])
    'waitcnt(clkfreq/40+cnt)
'      GraphicLcdSerial.str(strAddr)
  waitcnt(clkfreq/20+cnt)
'prints (S_R = 1) or erases (S_R = 0) a character to the screen
'at x_offset, y_offset. Automatically augments offsets for next write

PUB motor(speed,turn,cap,mode,estop)
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(9)
  GraphicLcdSerial.tx(speed)
  GraphicLcdSerial.tx(turn)
  GraphicLcdSerial.tx(cap)
  GraphicLcdSerial.tx(mode)
  GraphicLcdSerial.tx(estop)
PUB motorscreen
  GraphicLcdSerial.tx($7C)
  GraphicLcdSerial.tx(10)
PUB print_char(x,y,txt,S_R)|text_array_offset,j,dx,k,display,x_offset,y_offset,text_array

    display := 1 'for large display
    text_array_offset := (txt)*8

    x_offset := x
    y_offset := y

    text_array :=0' AtariFont.GetPtrToFontTable        
        
    repeat j from text_array_offset to text_array_offset+7
      k := text_array + j
      repeat dx from 0 to 7
        if(k & $80)
          SetPixel(x_offset, y_offset - dx,S_R)
          waitcnt(clkfreq/40+cnt)                        
        k <<= 1                        
        x_offset++        
      x_offset++
        
    if ((x_offset + 7) > (127 + display*32))
      x_offset := 0
      if (y_offset <= 7)                
         y_offset := 63 + display*64
      else
        y_offset -= 8
        
    if (display == 0)
      SetPixel(0,0,0)'cheat for smallo display
        

                  