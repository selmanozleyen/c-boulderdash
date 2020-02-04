# Boulderdash C Implementation

## Prerequisites
* The project requires allegro5 library to compile. [Here](https://github.com/liballeg/allegro_wiki/wiki/Quickstart) isa link to install the library on any platform.
* pkg-config 
* make
## Compilation
Once the requirements are met you can simply compile it by typing.
```
make
```
## Gameplay
### GUI
* T means time
* D means diamond or score
* M means merit
* L means level

### Controls
* Arrow keys for move.
* P for pause.
* R for resume.
* A for previous level.
* S for next level.

### Testing
* The level time and start merits are at max.
* If you want to test if the musics rhytm gets faster and you don't want to wait for the %90 of the level time you have to change the level0X file.
* The properties except the level file includes are hard coded so you will have to re-compile it.

### LEVELS
* Some levels are highly inspired by the original Comodore 64 game.
* Every screen is about 24X16 block.

###TIP FOR LEVEL DESIGN
Make a csv file and edit it via excel or libre calc the copy it to your level file.
