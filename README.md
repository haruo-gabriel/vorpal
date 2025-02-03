
VORPAL
======

**V**orpal **O**pen **R**eal-time **P**rocedural **A**udio **L**ayer is game
audio middleware focused on real-time procedural audio.

Build
-----

Before building, execute the following command:

``` bash
git config --global url."https://".insteadOf git://
```

This allows submodules with "git://" on their URLs to be updated.

Then, run:

```bash
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
make install
```

This will install library headers in
`/usr/local/include` and library files in
`/usr/local/lib` by default.

Then, build godot with the `vorpal-godot` module
in it. If, when trying to run the editor binary,
`libpdcpp` is not found, try running:

``` bash
export LD_LIBRARY_PATH=/path/to/vorpal/lib:$LD_LIBRARY_PATH
```
  * [ ] 
