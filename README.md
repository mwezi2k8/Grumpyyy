<h1 align="center">
  <img alt="Grumpyyy" width="700" src="images/hero.jpg"><br>
  Grumpyyy
</h1>

<h3 align="center">a tiny keychain that blinks at you, gets angry when you shake it, and then tells you to fuck off :D</h3>

## Why was it made?

i made this as a gift for my bro. i wanted to give him something i actually built myself instead of buying something, and a keychain that gets annoyed and tells you to fuck off felt like the perfect thing for him. i learned a lot making it, i2c, sensors, oled animations, kicad, soldering and fusion :D

## Demo

it chills and blinks normally. shake it for 2 seconds and the eyes turn angry. keep shaking and it stays angry. stop, and after 5 seconds it says FUCK OFF and calms back down. click the gif for the full video

<a href="media/testing-video.mp4"><img alt="demo" width="260" src="media/demo.gif"></a>

## How to use

just plug it into a laptop with a usb-c cable and shake it

| What you do | What it does |
| --- | --- |
| nothing | normal eyes, blinks every 2 to 5 sec |
| shake it for 2 sec | angry eyes with eyebrows, eyes tremble |
| keep shaking | stays angry |
| stop shaking for 5 sec | shows FUCK OFF for 2 sec, then back to normal |

## Faces

| Normal | Angry | Round | Text |
| --- | --- | --- | --- |
| <img alt="normal eyes" width="200" src="images/normal-eyes.jpg"> | <img alt="angry eyes" width="200" src="images/angry-eyes.jpg"> | <img alt="round eyes" width="200" src="images/round-eyes.jpg"> | <img alt="fuck off text" width="200" src="images/fuck-off.jpg"> |

the main firmware uses normal, angry and the text. every face also has its own separate sketch in `firmware/faces` so you can run them one by one

## How it works

the xiao esp32-c3 talks to both the oled and the mpu6050 over the same i2c bus (sda on d4, scl on d5). the mpu6050 is read directly over i2c without a library. when the keychain is still the total acceleration is about 1g, if it goes more than 0.6g away from that it counts as shaking. 2 seconds of shaking switches it to angry mode, and every shake while angry resets a 5 sec timer. when the timer runs out it shows the text and goes back to normal. the eyes are drawn with the adafruit ssd1306 and gfx libraries, the angry eyebrows are just black triangles drawn over the top of the eyes

## CAD

case designed in fusion 360, a base with a usb-c cutout and keyring tab, and a lid with a window for the oled

<img alt="case in fusion 360" width="700" src="images/case-fusion.jpg">

## Schematics

| Schematic | Wiring |
| --- | --- |
| <img alt="schematic" width="450" src="images/schematic.png"> | <img alt="perfboard wiring" width="450" src="images/perfboard-back.jpg"> |

made in kicad. its just 4 nets, 3v3, gnd, sda and scl, shared by the oled and the mpu6050. ad0 of the mpu6050 goes to gnd so its address is 0x68, the oled is 0x3c. everything is soldered on a small perfboard with wires on the back, no custom pcb

## Building one

1. get the parts from `BOM.csv`
2. wire it up on a breadboard first and check it works
3. in arduino ide install the esp32 boards and the adafruit ssd1306 + gfx libraries, select XIAO_ESP32C3 and turn on USB CDC On Boot
4. upload `firmware/Grumpyyy/Grumpyyy.ino`
5. solder it on perfboard using the schematic
6. print the case, put the board in and push the lid on

if the upload doesnt start, hold the B (boot) button while plugging it in

## Files

| Folder | Whats inside |
| --- | --- |
| `firmware/Grumpyyy` | main code with blinking, shake detection, angry eyes and text |
| `firmware/faces` | separate sketches for normal, angry, round eyes and the text |
| `kicad` | kicad project and schematic of the circuit |
| `cad` | fusion 360 script for the case and the STEP file of the base and lid |
| `images`, `media` | photos, gif and testing video |
| `JOURNAL.md` | build journal |
| `BOM.csv` | full parts list |

## BOM

the full list with specs and prices is in [BOM.csv](BOM.csv). main parts are 1x seeed studio xiao esp32-c3, 1x mpu6050 (gy-521), 1x 0.96 inch 128x64 ssd1306 oled, 1x small perfboard, some hookup wire, about 15 g of pla for the case and a key ring. total comes to around 1000 rupees
