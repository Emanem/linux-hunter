# linux-hunter
Prototype MH:W companion app for Linux, inspired by SmartHunter.

## Table of Contents

* [Usage](#usage)
* [UI](#ui)
* [Screenshots](#screenshots)
* [Status](#status)
* [Linux differences](#linux-differences)
  * [AoB structures](#aob-structures)
  * [Strings are longer](#strings-are-longer)
  * [Wine Complexities](#wine-complexities)
* [Root access](#root-access)
* [How to build](#how-to-build)
* [How to run](#how-to-run)
* [Credits](#credits)

## Usage
Running the application as `./linux-hunter --help` will produce the following:
```
Usage: ./linux-hunter [options]
Executes linux-hunter 0.0.5

-m, --show-monsters Shows HP monsters data (requires slightly more CPU usage)
-s, --save dir      Captures the specified pid into directory 'dir' and quits
-l, --load dir      Loads the specified capture directory 'dir' and displays
                    info (static - useful for debugging)
    --mhw-pid p     Specifies which pid to scan memory for (usually main MH:W)
                    When not specified, linux-hunter will try to find it automatically
                    This is default behaviour
    --debug-ptrs    Prints the main AoB (Array of Bytes) pointers (useful for debugging)
    --debug-all     Prints all the AoB (Array of Bytes) partial and full matches
                    (useful for analysing AoB) and quits; implies setting debug-ptrs
    --mem-dirty-opt Enable optimization to load memory pages just once per refresh;
                    this should be slightly less accurate but uses less system time
-r, --refresh i     Specifies what is the UI/stats refresh interval in ms (default 1000)
    --help          prints this help and exit

When linux-hunter is running:

'q' or 'ESC'        Quits the application
'r'                 Force a refresh
```

## UI
![UI](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/ui.jpg)

The UI itself is relatively simple. The first row contains the version and timings, useful to observe performance of this app (with _wall_, _user_ and _system_ timings).

The second row displays the MH:W session id (if any) and the owner of such session (player name).

The following rows will represent the players and the absolute/relative damage (currently they may display _NaN_ when not in a hunt).

If then you've enabled the `-m` (or `--show-monsters`), another pane will appear with the monsters currently tracked by the game and their current, total and % _HP_.

## Screenshots

![During Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/hunt0.jpg)
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

### Wine Complexities
I've also reached out to wine SMEs asking about the efforts of creating/porting such compation software. Their feedback has been:
```
What you are doing here is very fragile, even going from Windows to Windows. I realize it is a common thing
to do for game mods though if the game does not provide an API for such modifications.

How reliably memory patterns are replicated between Wine and Windows and even two different Windows versions
depends on how the allocations are made. If you are looking up pointers into the game's code in its DLLs and
EXE files they are very similar because the PE file is mmap'ed into the processes' address space. You have 
good chances of the absolute addresses to be identical.

If the game allocates a big blob of Heap memory in one go and fills it with data you should also be lucky.
If there are multiple independent heap allocations done by the game the patterns will start to look 
differently. Wine will not allocate smaller memory blobs than requested, but a heap allocation may be 
slightly larger, placed in different areas of the address space, etc. The exact details not only depend on
Wine, but also on the Linux kernel, linux libs etc.

Things will get even more spotty if the actual memory allocations are done by some Windows API functions.
I don't know the string APIs in detail, so the following example is just a hypothetical one: If the game
loads data from an XML file we pass the heavy lifting to the Linux libxml2 library. Its internal workings
are different from microsoft's msxml.dll so the layout of the loaded file will not look alike at all.

You can try to look into some observable allocation properties with functions like HeapSize and 
VirtualQuery. One thing worth exploring is finding memory allocations not by searching for magic patterns
in memory but hooking functions that the game uses to load the data. It may or may not work better.
```

## Root access
Unforutnately looks like CAPCOM and/or wine/Proton are protecting memory, hence this requires `sudo` access when running.

## How to build
You need to have `libncursesw5-dev` installed to compile (on Ubuntu is `sudo apt install libncursesw5-dev`) and that's it.
Once done, `make release` and you'll have your _linux-hunter_ ready to be running.

## How to run
Simply identify the MH:W _pid_, and then `sudo ./linux-hunter -p <pid>`; press `Esc` or `q` to quit.
Running with experimental option `--mem-dirty-opt` should reduce the performance impact by limiting scanning memory (on i7-8700k the CPU usage went from 9% to 2%) - but the stats per player may be slightly inaccurate (i.e. may be refreshes late).

There are some options to help out with debugging (such as `--debug-ptrs` and `--debug-all`), if you use those I suppose you have compiled it yourself hence should have knowledge of such options (you should have looked at the code by then).

## Credits
This work couldn't have been possible w/o previous work of:
* [AsteriskAmpersand](https://github.com/AsteriskAmpersand)
* [Marcus101RR](https://github.com/Marcus101RR)
* [r00telement](https://github.com/r00telement/SmartHunter)
* [sir-wilhelm](https://github.com/sir-wilhelm/SmartHunter)

