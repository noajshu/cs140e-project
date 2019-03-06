# Final Project Proposal
### *Nadin Ey and Noah Shutty*

## Introduction
We propose to implement better threads with the twist of RFID card-controlled policies.

One source of inspiration is the following suggested project:
*"Make a clean system that can sensibly blend pre-emptive, cooperative, and deadline-based run-to-completion threads (which do not need context switching)"*

We are interested in all of the concepts above, and also in adding auxiliary hardware. RFID could be a fun way to interact with the pi via card interaction events captured by the kernel. We imagine modifying the thread scheduling policies and/or priorities on the fly using thread badging. One idea is to require certain permissions to allow a program to run deadline-based threads, since these will be prioritized over other threads to meet the deadline. 

The scheduler may become somewhat complicated to support the three forms of context switching mentioned above. The different types of threads may interact nontrivially. Hopefully we get all of this functionality working and well-tested.


## Tasks
- extract / extend our threads lab into a clean starting point for further mods
- implement deadline-based threads
- implement preemptive threads via timer interrupts
- (optional) implement cooperative context switching

- Enable SPI on RPI and get communication to RC522 module


## Security Motivation
Some systems e.g. laptops used by some government or corporate employees require external hardware-based authentication before operation "badging in". We were interested in a setup where this kind of security could be integrated in the kernel so that, e.g., the `sudo` command would only be enabled when an authorized user presented their ID card. 


## Datasheets
Wiki page:
http://wiki.sunfounder.cc/index.php?title=Mifare_RC522_Module_RFID_Reader
MFRC522 Datasheet:
http://wiki.sunfounder.cc/images/c/c6/MFRC522_datasheet.pdf
"How to Use an RFID RC522 on Raspberry Pi" (sunfounder Wiki):
http://wiki.sunfounder.cc/index.php?title=How_to_Use_an_RFID_RC522_on_Raspberry_Pi


BCM2835 ARM peripherals -- SPI interface:
in https://github.com/dddrrreee/cs140e-win19/blob/master/docs/BCM2835-ARM-Peripherals.annot.PDF
Auxiliary interfaces are described starting on page 8. More details focusing on the SPI start on page 20.

## Hardware
We will stick to the raspberry pi model A+.
It would be cool to check out the esp8266 breakout if we can get one with soldered pins.

- SunFounder MFRC522 RFID Kit ($6.99):
https://www.amazon.com/gp/product/B07KGBJ9VG

