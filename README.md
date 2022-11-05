---
tags: 📝️/🌱️
aliases:
  -
cssclass:
---

# TITLE: [[SpiritBall]]
---
## Demo
![](images/demo.gif)
## Abstract
With 100 days until Christmas 2022, this project is set as a design challenge to create a Christmas decoration to be handed out as gifts, and as an excuse to learn and play with new tech toys and concepts.  This Readme is a copy of my notebook for this project.
## MOC
[[ESP32-S3]]
[[SpiritBall]]
[[KiCad]]
[[ESP Flasher]]
## Project Idea
- A Christmas ornament, or other holiday decoration in a ball form with addressable or patterned LED lighting.
- Primarily designed to be hung from a tree branch (plug in to light strand?)
- Based on [[ws812b]] LEDs.  
- Designed around multiple interlocking "leaves" and central management core.
	- See pic
		- ![img](images/Pasted%20image%2020220916093054.png)


## Design
### Mechanical
- Using the "multiple interlocking "leaves" strategy
	- Can we utilize card edge connectors?
	- Simpler, we could just leave the traces uncovered and solder bridge them together.
	- LEDs will be placed along the edge, firing normal to the leaf face.
		- First pass, only on one side, we can think about adding a second set on the reverse side later
		- LEDs should fire primarily in the direction of the next leaf, which should help with light diffusion
	- Do we need additional diffusion?
		- Likely not, given the LEDs will shine into the next leaf *AND* the abiliity of software to dim the LEDs, we shouldn't need mechanical diffusion
### Electrical
- Use a [[WS2812B]] [[SMD LEDs|LED]]
	- Likely this one, which I think is the same as on the ESP32-S3 Dev board
	- [WS2812B-Mini](https://jlcpcb.com/partdetail/Worldsemi-WS2812BMini/C527089)
	- Alternatively, this one which is the normal [[WS2812B]]
	- [WS2812B](https://jlcpcb.com/partdetail/Worldsemi-WS2812B_BW/C2761795)
- Control LEDs with [ESP32-S3 Module](https://jlcpcb.com/partdetail/3198296-ESP32_S3_WROOM_1N8/C2913198)
	- [Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
	- Likely need to spend some time figuring out how to use module standalone instead of on dev board
	- Can we power this from a standard christmas light strand?
		- How about a battery? 18650?
	- Technical things
		- EN/BOOT, Reset, Power On, UART/USB
			- A switch on EN would be a temporary contact to ground to reset the board, probably wanted on main board for a while
			- A siwtch on BOOT would allow the board to be reset into BOOTLOADER mode.  Which would then allow programming.  Assuming we are loading via USB/UART, which would otherwise control the EN and BOOT pins via transistors (see Automatic Bootloader for S3), we would never use this button.  Ergo, don't need it on board, we can pull a button (still not very useful) to the flash tool board to save space and parts.
			- So, onboard button for EN, no onboard button for BOOT, but we will utilize an external flash tool (usb/uart bridge) and we can stick a redundant EN button and BOOT button there.
			- In this scenario, we still need a pullup on EN.  BOOT can be left "floating" on the internal pullup.
- USB-C
	- [JLCPCB Link](https://jlcpcb.com/partdetail/Molex-1054500101/C134092)
	- [Molex Part](https://www.molex.com/molex/products/part-detail/io_connectors/1054500101)
	- [Molex Spec](https://www.molex.com/pdm_docs/ps/PS-105448-001-001.pdf)
	- USB/UART converter
### Software
## Work
- [[2022-09-26]] Created [SpiritBall Repo]() with first pass of schematic and PCB layout.  Tagging as [SpiritBall v0.1.0]()  This is intended as a minimum viable product with only a single plane of LEDs.  I still need to create board gerber files and run them through [[JLCPCB]] online tools to check for compatibility, as well as generate a BOM and place the order.
	- ![img](images/Pasted%20image%2020220926081628.png)
	- ![img](images/Pasted%20image%2020220926081646.png)
- [[2022-11-01]] - Success!  The first batch of 5 assembled boards came back last week (I had done some minor tocuch up work, swapped some parts for in-stock, etc).  I also used a [[ESP-Prog]] flash board to get things started, though I think I could eventually use the USB cable or even [[OTA]] updates.  **TODO** add pictures and video.  Re: software, I've been turned onto using [[RMT]] for controlling the LEDs by [[@Chris Lomont]], and it was a cinch to get up and running a simple rainbow chase.  Now the fun part - Making it look more appealing and less like rainbow unicorn vomit :) 
	- PS: Had a brief moment of panic - Playing with some LED patterns and decided to pull back the brightness so it didn't hurt my eyes.  I lowered the SATURATION when I meant to lower the VALUE in the HVS color representation.  This of course just made all the LEDs white, and full brightness, which spiked current draw up over 700mA.  Oops.  But the program runs at boot, and is powered through the PROG interface meant there wasn't enough power available to flash through UART.  Turns out the USB port on the [[SpiritBall]] works fine to power the board with some extra power.  Plugged in both USB and PROG interface (which saw maybe 10mA draw) and recovered the board.  I'm pretty happy with how this first revision is performing.
	- Also, had to shake off some rust to get example code up and running.  Here's what I did:
		```
		cd C:\projects\brendon\ESP32S3-Project-Template
		xcopy /e /i C:\Espressif\frameworks\esp-idf-v5.0\examples\get-started\hello_world ./
		idf.py set-target esp32s3
		(remove build dir if it complains)
		idf.py -p COM5 flash monitor
		```
- [[2022-11-05]] - I've been experimenting with different patterns and have got to the point I have things worth saving.  So I'm spending some time to update github.  Also switched over to C++ for most things, but I kept the [[RMT]] encoder in C because I didn't feel like re-writing it and it works well enough as is... Maybe that's a task for a later date.  Things I still need to do:
	- [ ] Settle on a handful of set patterns (including `OFF`)
	- [ ] Cycle through set patterns with onboard buttons
	- [ ] Add some sort of software over-current protection
	- [ ] Update via USB-C cable
	- [ ] Update via OTA (stretch)
	- [ ] Control patterns via BT or WiFi (stretch)

/Work
# ./[[SpiritBall]]
---

Tags: [[ESP32-S3]]

Reference:

Related:

Created: [[2022-09-16]]
