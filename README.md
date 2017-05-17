Runt-plumber is set of 
[runt](http://www.github.com/paulbatchelor/runt.git)
bindings for Soundplumber, the internal 
sound engine and API for the 
[Sporth](http://www.github.com/paulbatchelor/sporth.git) language. Runt-plumber
completely bypasses the parsing and lexing layer of Sporth, opting to use
the better designed parser for Runt to touch the underyling plumber engine. 
Combined with a custom data structure, Runt-plumber aims to eventually provide a system
better geared for realtime on-the-fly coding and abstraction. 

Realtime for the moment has been disabled. That will come back again soon!

Runt-plumber also comes with sporth.rnt, a runt dictionary containing some
handy words to use, including many words in Sporth. 

## Installation

At the moment, Runt-plumber has only been tested on Linux.

Runt-plumber needs the following dependencies:
- Runt
- Sporth 
- Soundpipe
- JACK

To compile simply run:

    make

Followed by this command as a local user:

    make install

Do *not* use sudo.

## Example

See the file "sine.rnt" and "test.rnt" for some examples. More documentation soon!
