# QUASAR 3D Gaussian Splatting

Uses the [QUASAR](https://github.com/quasar-gfx/QUASAR) framework to stream rendered [3D Gaussian Splatting (3DGS)](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/) scenes!

Right now, it can render and stream 3DGS scenes in the `.ply` format to a client (including mobile VR headsets!) over a real WiFi network!

This is more of a fun side project, so it's not really optimized for performance, so rendering/streaming code can be slow on large scenes. 
That being said, it should be able to render and stream most reasonably-sized scenes at 60 FPS on a modern desktop GPU (e.g. RTX 3060 or higher).

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

### 3DGS Viewer
```# in build/ folder
./gs_viewer --size 1920x1080 --ply <path to .ply file>
```
`test.ply` is provided in this repo as an example, but you may have to move back a little from the starting position to find it. 
This codebase should be able to load the `.ply` models/scenes found in the [original 3DGS project](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/), as well as any other `.ply` files that follow the same format.

### 3DGS Streamer
```
# in build/ folder
./gs_streamer --size 1920x1080 --ply <path to .ply file> --video-url <client IP address>:12345 
```

### 3DGS (ATW) Receiver
Only ATW is supported as the reprojection method for now.

You can either run `atw_receiver` in the [QUASAR](https://github.com/quasar-gfx/QUASAR) repo (docs: https://quasar-gfx.github.io/QUASAR/#atw),
or `ATWClient` in the [QUASAR-client](https://github.com/quasar-gfx/QUASAR-client) repo (docs: https://quasar-gfx.github.io/QUASAR/openxr.html#atw-client). 

**NOTE: If you are running on a VR headset, please add `--vr` in the command line arguments for `gs_streamer` and set `--size 3840x1080` (or whatever matches `videoSize` in the headset code).**

Remember to set/change the IP addresses in the command line arguments and in the headset code!

## Credit

3D Gaussian Splatting rendering code is borrowed from https://github.com/hyperlogic/splatapult and is modified and pasted into this repo. 
It's not fully integrated using QUASAR renderer classes yet, but it works! Thank you to the authors of the original code for their work!

## Citation
If you find this project helpful for any research-related purposes, please consider citing our paper:
```
@article{lu2025quasar,
    title={QUASAR: Quad-based Adaptive Streaming And Rendering},
    author={Lu, Edward and Rowe, Anthony},
    journal={ACM Transactions on Graphics (TOG)},
    volume={44},
    number={4},
    year={2025},
    publisher={ACM New York, NY, USA},
    url={https://doi.org/10.1145/3731213},
    doi={10.1145/3731213},
}
```
