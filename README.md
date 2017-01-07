# Runt-Plumber

A work in progress!

Runt-plumber is set of 
[runt](http://www.github.com/paulbatchelor/runt.git)
bindings for Soundplumber, the internal 
sound engine and API for the 
[Sporth](http://www.github.com/paulbatchelor/sporth.git) language. Runt-plumber
completely bypasses the parsing and lexing layer of Sporth, opting to use
the better designed parser for Runt to touch the underyling plumber engine. 
Combined with a custom data structure, Runt-plumber aims to provide a system
better geared for realtime on-the-fly coding and abstraction.

Runt-plumber also comes with sporth.rnt, a runt dictionary containing some
handy words to use, including many words in Sporth. 

## Installation

At the moment, Runt-plumber is Linux only (sorry, Nick!)

Runt-plumber needs the following dependencies:
- Runt
- Sporth (with JACK enabled)
- Soundpipe
- JACK

Optionally, my fork of the 
[kilo](http://www.github.com/paulbatchelor/kilo.git) provides a simple text
editor with a built in REPL for Runt. 

To compile simply run "make" followed by "sudo make install".

## A tutorial

### Built-in plumber words

Start up irunt, and load plumber in the following way:

    > "plumber" load

Make a new instance of plumber: 

    > plumb_new

Start up JACK (the equivalent of "sporth -b jack -c 2 -0 -S"):

    > 0 peak plumb_start

Compile some code:

    > "440 0.5 sine dup" 0 peak plumb_parse

Evaluate the code:

    > 0 peak plumb_eval

Cut it out:

    > "0 0" 0 peak dup plumb_parse plumb_eval 

Say good bye:

    > 0 peak plumb_stop
    > quit


### Using the Sporth dictionary

To save on typing, a dictionary has been made in the file 
*sporth.rnt*. These words are  designed to mimic the look and feel of Sporth
as closely as possible while still maintaining Runt-Sporth interoperability.

Below is how one could play and stop a sine in realtime. 
Open up irunt in the same directory as runt-plumber and do the following:

    > "sporth" load
    loading sporth...
    > hello
    > 440 0.5 2 fargs sine
    > do
    > mute
    > bye
    > quit

- **"sporth" load"** loads the dictionary *sporth.rnt*
- the world **hello** initializes and starts up JACK
- taking in a single argument, **fargs** is a word that makes it 
- easy to convert runt floats into sporth floats 
- the word **do** evaluates the code on the stack
- the word **mute** turns off the sound
- the word **bye** stop the audio and cleans up
- the word **quite** leaves irunt
