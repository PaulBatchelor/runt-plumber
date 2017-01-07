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

### Building new words 

This is too much typing and is quite slow and tedious. 
Defining words and procedures helps to build
up abstractions. The first thing to go will be to replace "0 peak" with
a word called *pd* :

    > [ : pd 0 ; ] 0 peak _pd set
    > pd p
    32474

Using this newly defined word, a new procedure can be made for generated a 
sine wave with a variable frequency:

    > [ : sine pd plumb_float "0.3 sine" pd plumb_parse ; ]


For added convenience, a word like "eval" could be an abbreviated form of 
plumb_eval, which automatically makes incoming code stereo:

    > [ : eval "dup" pd plumb_ugen pd plumb_eval ; ]

Now a sine can be constructed a lot faster :

    > 440 sine eval ;

More procedures can be added to make dialtones and to turn off sound:

    > [ : dial 440 sine 350 sine "add" pd plumb_ugen ; ]
    > [ : mute 0 pd plumb_float eval ; ]

### Using the Sporth dictionary

    Coming soon! 
