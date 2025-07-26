# QUASAR Gaussian Splatting

## Clone the Repository
```
git clone git@github.com:EdwardLu2018/QUASAR-gs.git --recursive
```

If you accidentally cloned the repository without --recursive-submodules, you can do:
```
git submodule update --init --recursive
```

## Building

```
mkdir build
cd build
cmake ..
cmake --build . -j $(nproc)
```

## Running
```
# in build/ folder
./gs_viewer <path to .ply file> # test.ply is provided as an example
```

## Credit

Gaussian Splatting rendering code is borrowed from https://github.com/hyperlogic/splatapult
