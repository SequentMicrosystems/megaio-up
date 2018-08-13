# megaio-up
--------------------------------------------
This is the command to control  [Raspberry Pi Mega-IO Expansion Card](https://www.sequentmicrosystems.com/megaio.html) on UP Board computer.

All code is tested on UP Board in ubilinux.
More about [UP Board](www.up-community.org)

## Usage
--------------------------------------------
```bash
~$ git clone https://github.com/SequentMicrosystems/megaio-up.git
~$ cd megaio-up/
~/megaio-up$ sudo make install
```

Now you can access all the functions of the Mega-IO board through the command "megaio"
If you clone the repository any update can be made with the following commands:
 
 ```bash
 ~$ cd megaio-up/
 ~/megaio-up$ git pull
 ~/megaio-up$ sudo make clean
 ~/megaio-up$ sudo make install
 ```
