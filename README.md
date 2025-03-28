# ha_energy

A Energy monitoring and control program for Linux in C that integrates with Home Assistant (hosted on the same Linux server), IMMETER three and single phase power meters for GRID power, and two custom 8-bit contollers that monitor a small (4KWh) solar energy hybrid banks for a house.

The 8-bit controllers (PIC18 K42 and Q84) integrate a FM80 charge controller for storage into the main battery bank, EM540 three-phase power meter for OFF-GRID inverter power, Renogy ROVER elite 40A and AC to DC 50V 1200W power supply for AC coupling of solar energy into a load shifting battery bank, with various coupling and interconnect components.

The project page is here. 

https://forum.allaboutcircuits.com/threads/fm80-solar-charge-controller-datalogger.194146/#post-1826013
FM80 solar charge controller datalogger
