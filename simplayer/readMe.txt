Sim(ple)Player 
a simple MP3/OGG file player coded in C99 and Raylib
-------------------------------------------------------------
note: .mp3/.ogg files are loaded from /home/$user/Music folder...

APP KEYBINDINGS
------------------------------------------------------------------
[ ESC ] : switch between vumeter / file library  mode

Vumeter mode (default):
------------------------------------------------------------------
[ cursor left / right ] : volume up / down by 5%
[ mouse click on progressbar area ]:  back and forth current audio track (seek +/-)
[ SPACE ] : play/stop song
[ I ]     : switch info between album & year and artist (default artist)
[ M ]     : mute audio volume
[ S ]     : shuffle play on / off (default on)
[ N ]     : play next song
[ Z ]     : play previous song

File Library:
note: files are alphabetically ordered (case sensitive)
-----------------------------------------------------------------
[ cursor up/down ( or mouse wheel ) ] : select file in library
[ page UP / DOWN ] : fast up or down scroll
[ HOME ]  : goes to first song 
[ END ]   : goes to last song
[ 0-9 ]   : start to type index number and goes directly to numbered song
[ DEL ]   : select the current playing song..
[ Enter ] : play selected file

Extra keybindings:
---------------------------------------------------------------------
[ V ]     : change spectrum visualizer color from mono to RGB (default mono)
[ ctrl + C ] : copy in clipboard the title - artist info...
[ Q ]     : leave app

// --------------------------------------------------------------
