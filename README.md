# Bedside Table Weather Station
An ESP8266/STM32G0-based LED matrix display designed to be a small-sized clock & weather station that can be placed on a bedside table.

## Why build this?
1. My wife has this habit that the first thing she does after wokeup is to fetch for her phone, ask siri about today's weather so she gets to know what to wear. A dedicated weather indicator makes this procedure simpler.
2. I just got a couple of this static monochrome 16x25 LED display module, with an STM32G070KBU6 that without its serial-wire debug interface disabled. The module is possibly scrapped from some electric bicycle since its default input voltage is up to 20V. Given reason 1, this module may just be the right component.
3. I just bought a Bambu Lab H2D, the performance and multifunctionality of this thing may just allow me to build a deligate enclosure for the weather station.

## Features (planned)
1. Weather + time acquisation from internet.
2. Vibration detect (slapping).
3. Brightness adaption.
4. Special effect pattern when not activated.

## First step: Reverse-Engineering the module
Since this is a pre-made module, first step is to reverse-engineer.

Objectives:
1. Finding power scheme - done
2. Finding MCU debug interface - done
3. Finding connections between MCU and LED drivers - done
4. Finding algorithms used to drive the LEDs - done

This is a static LED matrix - meaning in theory there is no need to do any form of scan, which then implies this module can reach ridiculously high refresh rates - 400 monochrome LEDs can be updated at 40kHz if a 16MHz shift clock is used.

The original firmware onboard automatically runs a test pattern of lighting all the LEDs at three different brightnesses. The algorithm used can be identified by probing the pins with an oscilloscope, and it turns out The original firmware does the grayscale control through brute-force:

1. LAT signal is at 40kHz.
2. CLK signal is at 16MHz, and non-stopping.
3. DAT signal is at ~156Hz, all-low for full brightness. This is also the fps.
4. OE signal is always enabled.

Based on these observations, it's quite straight forward to calculate the update-per-frame as $40000 / 156 \approx 256$. This indicates the maximum grayscales this algorithm can reach is 8-bits. This is similar to Binary Code Modulation (BCM) with the only difference that in real BCM non-stopping update is not required, you do not need to update LEDs every tick, you only need to update 8 times at the correct timing.

This is actually more reasonable than original BCM because managing data transfer at the correct timing brings additional complexity to the system. Transferring same data many many times costs nothing but power consumption, which is pointless in an LED-orinted product. And by the non-stoppping clock signal we can conclude the original firmware utilizes SPI/I2S+DMA. DMA is set to circular mode with half-complete and transfer-complete events to implement a ping-pong update scheme.




