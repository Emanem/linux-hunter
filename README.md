# linux-hunter
Prototype MH:W companion app for Linux, inspired by SmartHunter.

## Table of Contents

* [Screenshots](#screenshots)
* [Status](#status)
* [Linux differences](#linux-differences)
 * [AoB structures](#aob-structures)
 * [Strings are longer](#strings-are-longer)
* [Root access](#root-access)
* [How to build](#how-to-build)
* [How to run](#how-to-run)
* [Credits](#credits)

## Screenshots

![Before Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/start_hunt.jpg)
![Mid Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/mid_hunt.jpg)
![End Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/end_hunt.jpg)
![Back from Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/back_gathering.jpg)

## Status
Current code/logic is highly prototype and unoptimized - please refrain from using it, or use it at your own risk.

## Linux differences
Following the main differences I had to overcome to port the logic of SmartHunter to Linux; considering the challenges below, I think I've been lucky so far.

### AoB structures
Some AoB structures (_Array of Bytes_) that are used to lookup pointers are slightly different.
For example, look at [PlayerName AoB](https://github.com/Emanem/linux-hunter/blob/6b397574ff51d5c37b2dc4f2ec32e6558901a807/src/patterns.cpp#L31): that doesn't work on Linux, but then I had to modify it and a similar version [PlayerNameLinux AoB](https://github.com/Emanem/linux-hunter/blob/6b397574ff51d5c37b2dc4f2ec32e6558901a807/src/patterns.cpp#L61) does instead the trick.

Other AoBs such as [PlayerDamage](https://github.com/Emanem/linux-hunter/blob/6b397574ff51d5c37b2dc4f2ec32e6558901a807/src/patterns.cpp#L41) can be found straight away.

### Strings are longer 
When reading the players' names I have to [add one byte](https://github.com/Emanem/linux-hunter/blob/6b397574ff51d5c37b2dc4f2ec32e6558901a807/src/main.cpp#L149); compare the similar logic in [SmartHunter](https://github.com/sir-wilhelm/SmartHunter/blob/7fa3d5a30a653f3587d3ba32afec195224690b9c/SmartHunter/Game/Helpers/MhwHelper.cs#L298).

## Root access
Unforutnately looks like CAPCOM and/or wine/Proton are protecting memory, hence this requires `sudo` access when running.

## How to build
You need to have `libncursesw5-dev` installed to compile (on Ubuntu is `sudo apt install libncursesw5-dev`) and that's it.
Once done, `make release` and you'll have your _linux-hunter_ running.

## How to run
Simply identify the MH:W _pid_, and then `sudo ./linux-hunter -p <pid>`. Press `Ctrl+C` to quit.
There are some options to help out with debugging, if you use it I suppose you have compiled it yourself hence should have knowledge of such options.

## Credits
This work couldn't have been possible w/o previous work of:
* [AsteriskAmpersand](https://github.com/AsteriskAmpersand)
* [Marcus101RR](https://github.com/Marcus101RR)
* [r00telement](https://github.com/r00telement/SmartHunter)
* [sir-wilhelm](https://github.com/sir-wilhelm/SmartHunter)

