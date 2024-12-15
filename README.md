
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
```
