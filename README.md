# QUASAR Gaussian Splatting

Uses the [QUASAR](https://github.com/quasar-gfx/QUASAR) rendering engine to render and stream Gaussian Splats.

## Clone the Repository
```
git clone --recursive git@github.com:EdwardLu2018/QUASAR-gs.git
```

If you accidentally cloned the repository without `--recursive`, you can do:
```
git submodule update --init --recursive
```

## Install Dependencies
Please follow the instructions here: https://quasar-gfx.github.io/QUASAR/#install-dependencies

## Building

```
mkdir build; cd build
cmake ..; cmake --build . -j $(nproc)
```

## Running

### GS Viewer
```# in build/ folder
./gs_viewer --size 1920x1080 <path to .ply file>  # test.ply is provided as an example
```

### GS Streamer
```
# in build/ folder
./gs_streamer --size 1920x1080 <path to .ply file> --video-url <client IP address>:12345
```

### ATW Receiver
You can either run `atw_receiver` in the [QUASAR](https://github.com/quasar-gfx/QUASAR) repo (docs: https://quasar-gfx.github.io/QUASAR/#atw),
or `ATWClient` in the [QUASAR-client](https://github.com/quasar-gfx/QUASAR-client) repo (docs: https://quasar-gfx.github.io/QUASAR/openxr.html#atw-client).
Remember to set/change the IP addresses!

## Credit

Gaussian Splatting rendering code is borrowed from https://github.com/hyperlogic/splatapult.
