# UHF-RFID-Inventory-System (Emma's RFID Thing)

https://github.com/user-attachments/assets/c475d00b-563d-4544-90bf-386b262ad7c9

_Goofin'_

<img width="250" alt="image" src="docs/images/1.png" /><img width="250" alt="image" src="docs/images/2.png" /><img width="250" alt="image" src="docs/images/3.png" />

_The screen indicates that 6 out of 8 known items have been found. The missing items are identified as my macbook charger (CHRG) and my label maker (LABEL)._

---

## Abstract

Here I present a low cost RFID Inventory System for tracking my work tools. The USB-powered device sits in my car and tells me when I forget a tool on the job site. Costing around $120 in materials, this self contained device is capable of reliably identifying UHF RFID tags from 10-20 feet away, even when they are occluded by other (non metal) objects. This makes it ideal for detecting if an item is in my car or not. If all of my tagged tools are present and accounted for, a check mark is displayed on the screen, indicating I'm good to go. If any tools are missing, an X mark and a list of missing tools is shown. This allows me to quickly assess if I have all of my tools and has already prevented several expensive mistakes.

## Motivation

I was recently at TopGolf, which uses RFID, alongside other technologies, for realtime tracking of golf balls as they fly through the air. I found it really amazing that they could differentiate between dozens of moving objects, across a giant driving range, all in real-time. What I found even more amazing was that this required no active electronics in the balls themselves, just a passive RFID tag.

This led me to think about the actual limits of RFID technology. I was particularly interested in what concepts could be created around the idea of tracking tagged objects from a distance. I had just lost an expensive work tool and was pretty bummed out about it, so the prospect of an RFID inventory system sounded great.

**Here's the concept: I put RFID stickers on the stuff I care about, and a reader in my car tells me me if any items are missing.**

## Background

<img width="500" src="docs/images/explainer.gif" />

RFID uses the fact that radio waves can both bounce off of and be absorbed by objects depending on material properties.

<details>
<summary>An Aside: The history of RFID</summary>

<img width="250" src="docs/images/soviet_thing.png" />

During the tail end of World War II, the United State's Ambassador to the Soviet Union received a carved wooden emblem as a gift. Innocuous enough and hand delivered by Moscow schoolchildren, this eagle carving was proudly displayed in the Ambassador's Moscow residence. Hidden inside, however, was a sophisticated listening device that used the principal behind RFID to encode messages in radio wave reflections. The device, which became known as the Thing, needed no battery or power source. Instead, it relied on a radio antenna stationed nearby to broadcast a signal. Radio waves can both bounce off of and be absorbed by objects. A microphone inside the carving would vary how much of the signal gets absorbed, which impacts the signal reflected back. By detecting variations in signal's reflection, audio information could be decoded. This system was used to spy on the United States government, without detection, for nearly 7 years.

</details>

An RFID reader sends out a 860-960MHz radio signal, called the carrier wave. Some of this signal is absorbed by the tag's antenna and converted into power for a tiny computer on board. By changing the antenna's properties to selectively absorb more or less of the carrier wave, the reflected wave can be changed. This change in the reflection back is detected by the reader. This means the tag's computer can "send" data back without using any additional power.

The on-tag computer implements EPC, a communication and data storage protocol centered around reducing radio signal collisions. Further Reading: Anti Collision Algorithms

RFID's has become ubiquitous, with a wide range of uses ranging from supply chain and warehouse inventory management, to microchipping your pets and stopping retail theft.

Because of this, our friends in Shenzhen, China have discovered ways of making this technology surprisingly accessible. Sample boards and high range antenna kits are widely available on Chinese Marketplaces like AliExpress.

---

## Electronics

<img width="500" src="docs/images/wiring_diagram.png" />

| ESP32 Pin | Destination                 |
| --------- | --------------------------- |
| +5V       | VCC on RFID and VCC on OLED |
| GND       | GND on RFID and GND on OLED |
| GPIO 14   | RFID TX                     |
| GPIO 26   | RFID RX                     |
| GPIO 33   | OLED SDA                    |
| GPIO 35   | OLED SCL                    |

### Parts List

1. RodinBell M300 Series UHF RFID Reader

The easiest and cheapest way to source this chip is through a development board. You can find these on [AliExpress](https://www.aliexpress.us/w/wholesale-r200-uhf-rfid-reader-module.html) sold as R200 or R300 boards. [Here is what I purchased](https://www.aliexpress.us/item/3256810183614966.html).

2. Circularly polarized antenna

Many sample boards have the option of being sold with an antenna. I chose a 6 dBi circularly polarized antenna that came in a bundle with my reader board.

3. OLED Display

I chose a 2.42" SSD1309 128x64 OLED Display for size and cost. The size of this display board is around the same footprint as the RFID board and ESP32.

4. ESP32

Any microcontroller should work though.

I was able to source the RFID reader board and antenna together from AliExpress for just under $100. I got the display from Amazon for $15 and the ESP32 for $5. Coming in at $120 total, this device is surprisingly economical for its functionality.

## Hardware

<img width="500" src="docs/images/case.png" /> <img width="250" src="docs/images/case.gif" />

The electronics are held in a 3D printed case. STL files are available. If you'd like the (disorganized) Fusion 360 designs, send me an email.

## Software

<img width="250" src="docs/images/checkmark.png" />

The ESP32 software only needs to periodically send inventory commands to the reader and listen for tag data back in response. The RFID board handles all of the actual RFID inventorying, collision avoidance, and nitty-gritty communication details. All the ESP32 has to do is issue "start inventory" commands and wait for tags to be detected. By default, the device will scan for all saved tags in the vicinity. Once all tags are read, the device shows a check mark and puts the processor to sleep. If, instead after 5 minutes, not all saved tags have been found, an X mark is instead displayed alongside a list of missing tools.
