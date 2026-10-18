Sim(ple)Player 
a simple MP3/OGG file player coded in C99 and Raylib
-------------------------------------------------------------
note: .mp3/.ogg files are loaded from /home/$user/Music folder...

Global keybindings:
--------------------------------------------------------------------------------
[ ESC ] : switch between vumeter / file library  mode
[ SPACE ] : play/stop song
[ I ]     : switch info between album & year and artist (default artist)
[ M ]     : mute audio volume
[ S ]     : shuffle on : file list will be shuffled / Off: ordered list by filesystem loading
[ N ]     : play next song in the list
[ Z ]     : play previous song in the list
[ F1 or KP+ / F2 or KP- ] : volume up / down by 5%
[ cursor left/right ] : back and forth current track time position (seek +/-)
[ V ]     : change spectrum visualizer color from mono to RGB (default mono)
[ ctrl + C ] : copy in clipboard the title - artist info...
[ Q ]     : leave app
--------------------------------------------------------------------------------

File Library:
note: files are alphabetically ordered (case sensitive)
--------------------------------------------------------------------------------
[ cursor up/down ( or mouse wheel ) ] : select file in library
[ page UP / DOWN ] : fast up or down scroll
[ HOME ]  : goes to first song 
[ END ]   : goes to last song
[ 0-9 ]   : start to type index number and goes directly to numbered song
[ DEL ]   : select the current playing song..
[ Enter ] : play selected file
--------------------------------------------------------------------------------
