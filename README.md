# inductive_sledgehammer
ATMEGA32U4 code for the physical Sledgehammer device developed at TUM
Developed by Julian Brunner and Manuel Eberl

The wooden pedestal was milled out of solid oakwood by a carpenter. The sketch that was sent to the carpenter as a specification is included (annotated in German). The hammer itself is a standard 1.25 kg mini-sledge from a DIY shop. The microcontroller board used is a ProMicro Sparkfun (16 MHz, 5 V) that was programmed using the Arduino libraries.

The pedestal contains three 10 mm borings extending to 3 mm underneath the surface. A single coil is glued into the end of each of these borings. The microcontroller measures the time it takes for a change from LOW to HIGH sent from an output pin to register at an input pin attached to the output pin through the coil.

