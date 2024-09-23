
VORPAL
======

**V**orpal **O**pen **R**eal-time **P**rocedural **A**udio **L**ayer is game
audio middleware focused on real-time procedural audio.

Build
-----

Before building, use the command:

``` bash
git config --global url."https://".insteadOf git://
```

to enable updating submodules that use "git://" in their URLs.

```bash
git submodule update --init
mkdir build
cd build
cmake ..
make
```
