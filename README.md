# QUASAR Gaussian Splatting

Uses the [QUASAR](https://github.com/quasar-gfx/QUASAR) rendering engine to render and stream Gaussian Splats.

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
### Streamer
```
# in build/ folder
./gs_viewer --size 1920x1080 <path to .ply file> --pose-url 0.0.0.0:54321 --video-url 127.0.0.1:12345  # test.ply is provided as an example
```

### Receiver
You can either run `atw_receiver` in the [QUASAR](https://github.com/quasar-gfx/QUASAR) repo (docs: https://quasar-gfx.github.io/QUASAR/#atw), 
or `ATWClient` in the [QUASAR-client](https://github.com/quasar-gfx/QUASAR-client) repo (docs: https://quasar-gfx.github.io/QUASAR/openxr.html#atw-client). 
Remember to change the IP addresses!

## Credit

Gaussian Splatting rendering code is borrowed from https://github.com/hyperlogic/splatapult
