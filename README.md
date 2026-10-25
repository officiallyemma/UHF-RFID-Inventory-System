# UHF-RFID-Inventory-System

https://github.com/user-attachments/assets/c475d00b-563d-4544-90bf-386b262ad7c9

<img width="250" alt="image" src="docs/images/1.png" />
<img width="250" alt="image" src="docs/images/2.png" />
<img width="250" alt="image" src="docs/images/3.png" />

---

## Abstract

Here I present a low cost RFID Inventory System for tracking my work tools. The device lives in my car and tells me, before I've left, when I've forgotten a tool. Built around the RodinBell M300 Series UHF RFID Development Sample, this self contained device is capable of reliably reading UHF RFID tags from 10-20 feet away, even when they are occluded by other objects. This makes it ideal for detecting if an item is in my car or not.

## Motivation

I was recently at TopGolf, which uses RFID (alongside other technologies) for realtime tracking of golf balls as they fly through the air. I found it really amazing that they could differentiate between dozens of moving objects, across a giant driving range, all in real-time. What I found even more amazing was that this required no active electronics in the balls themselves, just a passive RFID tag.

This led me to think about the actual limits of RFID technology. I was particularly interested in what concepts could be created around the idea of tracking objects from a distance. I had just lost an expensive work tool and was pretty bummed out about it, so the prospect of an RFID inventory system sounded great. I put stickers on the stuff I care about, and a reader in my car tells me what, if anything, I'm missing. It's a great idea on paper, but its feasibility will depend on how high powered an RFID reader I can actually get my hands on for cheap.

### An Aside: How RFID Works

During the tail end of World War II, the United State's Ambassador to the Soviet Union received a carved wooden emblem. Innocuous enough and hand delivered by the Boy Scout's of Moscow, this eagle carving was proudly displayed in the Ambassador's residence in Moscow. Hidden inside lied a sophisticated listening device that some consider to be the first use of RFID-like technology. The listening device was able to spy on the United States government, without detection, for nearly 6 years. The recorder, which became known as the Thing, needed no battery or power source. Whenever there was an important meeting at the Ambassador's home, the USSR would park a van across the street. This van contained a radio antenna which broadcasted a signal. The antenna inside the carving would absorb some of the signal and, most importantly, allow parts of it to reflect back to the van to be recorded. Audio from the Ambassador's home was encoded as small differences in the frequency of the radio wave getting reflected back towards the van.

<img width="500" src="docs/images/explainer.gif" />

An RFID reader sends out a 860-960MHz radio signal, called the carrier wave. Some of this signal is absorbed by the metal antenna of the tag and converted into power for a tiny computer inside the tag. By absorbing some parts and allowing others to reflect back, the tag's computer can "send" data back, without using any additional power.

In other words, the reader sends a signal and the tag bounces that signal back. This allows for wireless communication, without any additional power required for the tags. The computer allows for a simple data storage and communication protocol centered around reading/writing data and collision avoidance.

---
