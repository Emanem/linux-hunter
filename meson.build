project('linux-hunter', 'cpp')

linux_hunter_source = files(
    'src/events.h',
    'src/fdisplay.cpp',
    'src/fdisplay.h',
    'src/hashtext_fmt.h',
    'src/main.cpp',
    'src/memory.h',
    'src/mhw_lookup.cpp',
    'src/mhw_lookup.h',
    'src/mhw_lookup_monster.h',
    'src/offsets.h',
    'src/patterns.cpp',
    'src/patterns.h',
    'src/timer.h',
    'src/ui.cpp',
    'src/ui.h',
    'src/utils.cpp',
    'src/utils.h',
    'src/vbrush.h',
    'src/wdisplay.cpp',
    'src/wdisplay.h',
    'src/memory.cpp',
)

notcurses_lib = dependency('ncursesw')

executable(
    'linux-hunter',
    linux_hunter_source,
    dependencies : notcurses_lib,
    install : true,
)
