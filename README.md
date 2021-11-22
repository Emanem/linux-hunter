# linux-hunter
Prototype MH:W companion app for Linux, inspired by SmartHunter.

## Table of Contents

* [Supported Versions](#supported-versions)
* [Usage](#usage)
* [UI](#ui)
* [Vulkan Overlay](#vulkan-overlay)
* [Screenshots](#screenshots)
* [Status](#status)
* [How it works](#how-it-works)
* [Linux differences](#linux-differences)
  * [AoB structures](#aob-structures)
  * [Strings are longer](#strings-are-longer)
  * [Wine Complexities](#wine-complexities)
* [Root access](#root-access)
* [How to build](#how-to-build)
* [How to run](#how-to-run)
* [Credits](#credits)
* [Changelog](#changelog)

## Supported Versions

* 15.11.01 - Fully Supported 
* 15.11.00 - Fully Supported (this is deprecated by CAPCOM)
* 15.10.00 - Fully Supported (this is deprecated by CAPCOM)
* 15.02.00 - Fully Supported (this is deprecated by CAPCOM)
* 15.01.00 - Fully Supported (this is deprecated by CAPCOM)
* ~~14.02.00 - Fully Supported~~
* ~~14.01.00 - Fully Supported~~
* ~~14.00.00 - Fully Supported~~
* ~~13.50.01 - Fully Supported~~
* ~~13.50.00 - Fully Supported~~

## Usage
Running the application as `./linux-hunter --help` will produce the following:
```
Usage: ./linux-hunter [options]
Executes linux-hunter 0.1.3

-m, --show-monsters     Shows HP monsters data (requires slightly more CPU usage)
-c, --show-crowns       Shows information about crowns (Gold Small, Silver Large and Gold Large)
-s, --save dir          Captures the specified pid into directory 'dir' and quits
-l, --load dir          Loads the specified capture directory 'dir' and displays
                        info (static - useful for debugging)
    --no-direct-mem     Don't access MH:W memory directly and dynamically, use a local copy
                        via buffers - increase CPU usage (both u and s) at the advantage
                        of potentially slightly less inconsistencies
-f, --f-display f       Writes the content of display on a file 'f', refreshing such file
                        every same iteration. The content of the file is a 'wchar_t' similar
                        to the UI, having special '#' as escape character to denote styles
                        and formats (see sources for usage of '#' escape sequances)
                        It is heavily suggested to have file 'f' under '/dev/shm' or '/tmp'
                        memory backed filesystem
    --mhw-pid p         Specifies which pid to scan memory for (usually main MH:W)
                        When not specified, linux-hunter will try to find it automatically
                        This is default behaviour
    --debug-ptrs        Prints the main AoB (Array of Bytes) pointers (useful for debugging)
    --debug-all         Prints all the AoB (Array of Bytes) partial and full matches
                        (useful for analysing AoB) and quits; implies setting debug-ptrs
    --mem-dirty-opt     Enable optimization to load memory pages just once per refresh;
                        this should be slightly less accurate but uses less system time
    --no-lazy-alloc     Disable optimization to reduce memory usage and always allocates memory
                        to copy MH:W process - minimize dynamic allocations at the expense of
                        memory usage; decrease calls to alloc/free functions
-r, --refresh i         Specifies what is the UI/stats refresh interval in ms (default 1000)
    --no-color          Do not use colours when rendering text (useful on distro which can't
                        handle ncurses properly and end up not displaying text)
    --compact-display   Makes the output take less vertical space by removing unnecessary
                        sections and line breaks. It comes in handy when pairing linux-hunter
                        with vkdto (see https://github.com/Emanem/linux-hunter#vulkan-overlay)
 
    --help              prints this help and exit

When linux-hunter is running:

'q' or 'ESC'            Quits the application
'r'                     Force a refresh
```

## UI
![UI](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/ui.jpg)

The UI itself is relatively simple. The first row contains the version and timings, useful to observe performance of this app (with _wall_, _user_ and _system_ timings).

The second row displays the MH:W session id (if any) and the owner of such session (player name).

The following rows will represent the players and the absolute/relative damage (currently they may display _NaN_ when not in a hunt).

If then you've enabled the `-m` (or `--show-monsters`), another pane will appear with the monsters currently tracked by the game and their current, total and % _HP_.

## Vulkan Overlay
Another way to use _linux-hunter_ is to create a _status_ output display file (`-f` or `--f-display`) and have a utility such as [vkdto](https://github.com/Emanem/vkdto) to read such file and update the main _MH:W_ window with overlay.

One could setup the _MH:W_ launch window on Steam as following:
```
VKDTO_HUD=1 VKDTO_FILE=/dev/shm/linux-hunter %command%
```
and then, once the game is up and running, execute _linux-hunter_ from the terminal with following options
```
sudo ./linux-hunter -m -f /dev/shm/linux-hunter
```
This way _vkdto_ will dynamically display the overlay with the content from _linux-hunter_ and you will see it without needing to keep the _terminal_ window in foreground (see below a screenshot with both overlay in action in foreground and background _linux-hunter_ in the terminal).
Please note that the _status_ file should be created on a _memory_ device (reccomended `/dev/shm` or `/tmp`) - otherwise this may overutilize the physical filesystem.

Currently _vkdto_ is still in alpha stages and you can't modify some options such the text size - please refer to [vkdto](https://github.com/Emanem/vkdto) github page for more info about it.

## Screenshots

![vkdto Overlay](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/vkdto_overlay.jpg)
![During Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/hunt0.jpg)
![Before Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/start_hunt.jpg)
![Mid Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/mid_hunt.jpg)
![End Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/end_hunt.jpg)
![Back from Hunt](https://raw.githubusercontent.com/Emanem/linux-hunter/master/pics/back_gathering.jpg)

## Status
Current code/logic is somehow prototype and partially optimized - please use it at your own risk.

## How it works
_linux-hunter_ primarily operates by loading the _entire_ MH:W memory address space into its own; it then scans memory to find some _magic_ patterns. When such patterns are found, it then goes into a loop and keeps on _navigating_ those patterns by de-referencing memory addresses and interpreting those according to the MH:W memory layout (i.e. player, monsters and session information).

_linux-hunter_ will perform such memory navigation every _n_ time and then display on the terminal UI required information.

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
The most optimized way to run _linux-hunter_ would be `sudo ./linux-hunter -m`; this way you would start it using both low CPU and memory, plus displaying _monsters_ information. In this case _linux-hunter_ will try to find MH:W _pid_ (if this fails to find the pid, you can use the `--pid <pid>` option).
Once running press `Esc` or `q` to quit.

There are some options to help out with debugging (such as `--debug-ptrs` and `--debug-all`), if you use those I suppose you have compiled it yourself hence should have knowledge of such options (you should have looked at the code by then).

## Credits
This work couldn't have been possible w/o previous work of:
* [AsteriskAmpersand](https://github.com/AsteriskAmpersand)
* [Marcus101RR](https://github.com/Marcus101RR)
* [r00telement](https://github.com/r00telement/SmartHunter)
* [sir-wilhelm](https://github.com/sir-wilhelm/SmartHunter)
* [Haato](https://github.com/Haato3o/HunterPie)

## Changelog

* 0.1.3
  - Added support for _Crown_ when avaiable (by [pr3martins](https://github.com/pr3martins))
* 0.1.2
  - Changes to support release 15.01.00 (Fatalis update)
* 0.1.1
  - Added option to remove colors when drawing data
* 0.1.0
  - Made by default `lazy-alloc` and `direct-mem` as true, this is the most optimized setup
* 0.0.8
  - Added option (`-f` or `-f-display`) to output the display as a `wchar_t` file so that can be read by other processes independently. This should work along the lines of _/procfs_ filesystem and it's hevaily reccomended to set the value of this option to a _ramfs_ type of directory (i.e. like '/dev/shm/linux-hunter.out' or '/tmp/linux-hunter.out')
* 0.0.7
  - Added experimental option (`-d`) to stop copying memory segments from MH:W process to _linux-hunter_; the latter now queries and _navigates_ MH:W memory _on-the-fly_ 
* 0.0.6
  - Support latest version of MH:W
  - Lookup monster data via pointers (as [HunterPie](https://github.com/Haato3o/HunterPie) does) - should be better for Linux
  - Improved UI elements and some fonts/colors
  - Added 'LobbyStatus' lookup (works on Linux as it is)

