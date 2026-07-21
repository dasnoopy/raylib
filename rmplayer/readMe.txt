rmPlayer: a simple MP3/OGG file player made with C + Raylib
-------------------------------------------------------------------
Some settings can be changed using a configuration file:
at first execution of app, /home/user$/.local/share/rmplayer folder
will be created. Default settings will be used.

If you want some customization, create a minimal rmplayer.cfg file
in that folder:

(note: bool [true/false] enable/disable option)

//---------------------------------------------------------------
// simple rmplayer.cfg
//---------------------------------------------------------------

[player]
isPlay=true                    // start autoplay at start
isShuffle=true                 // Shuffle at start
isMini=true                    // miniview mode
isVumeter=true                 // false: amplitude audio analyzer  
                               // true: use digital 20bars vumeter
musicDir=/where/your/Music     //set music folder where your mp3 files are located

[style]
accentColor=139,229,157,255    // application accent color
titleFnt=fonts/rmplayerdot.otf // custom ttf/otf font for MP3 infobar
dgtEffect=true                 // shadow text for song time and volume value

// -------------------------------------------------------------

APP KEYBINDINGS:

[ cursor up/down ( mouse wheel )] : select file in library
[ Enter ] : play selected file
[ Page UP/DOWN ] : volume UP / DOWN
[ cursor left/right] : audio seek -10sec/+10sec
[ SPACE ]: play/stop song
[ 1 to 4 ] : set windows position to center, bottom/left , bottom/middle, bottom/right
[ M ] : mute audio
[ S ] : shuffle play on / off
[ R ] : repeat song on / off
[ N [ : play next song
[ P ] : play previous song
[ X ] : goto current played song (filelist)
[ I ] : switch title info between ID3 tags and file details.
[ F ] : show / hide file library
[ Q ] : leave app

// --------------------------------------------------------------
