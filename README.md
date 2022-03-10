# albafetch ~by alba4k

![intro](images/albafetch.png)


albafetch is a simple, fast system fetching program. It prints many info about the system in way less than a second. I decided to make this as a challenge for myself and since I found neofetch too slow (which is understandable from a 10k+ lines shell script).

**It was made, and only works, on Arch Linux, due to a dependency for pacman**

Here is a time comparison (exact execution times change between machines and runs):

![neofetch](images/time_neofetch.png)
![albafetch](images/time_albafetch.png)

## Dependencies:
* whoami (username)
* pacman (packages)
* wc (packages)
* uname (kernel, os arch)
* free (memory)
* grep (memory)
* awk (memory)

All of those will be present by default on a normal Arch Linux installation.

* lsb-release (getting the distro name)

lsb-release will need to be installed

You will be able to use this script by downloading an executable from the [releases page](https://github.com/alba4k/albafetch/releases) or by compiling it yourself. While compiling might appear more complicated, it will allow you to **customize the output**.

# Compiling:
```shell
$ git clone https://github.com/alba4k/albafetch
$ cd albafetch/
$ make
```
An executable file will appear in `albafetch/`

# Installing (automatically compiles)
```
$ git clone https://github.com/alba4k/albafetch
$ cd albafetch/
# make install
```
`# make install` needs to be ran as root to acces `/usr/bin`. It will compile `albafetch/src/albafetch.c` to this directory.

# Customizing
```
$ git clone https://github.com/alba4k/albafetch
$ cd albafetch/src/ 
```
You can change some settings in `albafetch/srtime_neofetchc/config.h`, or by directly changing the main `albafetch/src/albafetch.c` file. You will need to recompile/reinstall after configuring.

---

### Aaron Blasko, March 2022