# Brief
This project is created on top of openGL, and is ready to execute your particle shader.
The repo by itself contains only sources, but you can find the build with example shader in the release archieve.

# Example of what you can create
![](./video.gif)

# How to work with it?
So, at first you need to look at `shaders/1.glsl` and notice how shader is construced.
And when you are ready to write your own shader, you can modify whole vertex shader, but in most cases it is recommended to write custom code only in `mutable section`.

# Usage
#### of the latest version (1.0.1)
- press button `1` to load vertex shader from path shaders/1.glsl
- press button `2` to load vertex shader from path shaders/2.glsl
- and so on...

- `ctrl+s` - save particles state
- `ctrl+l` - load particles state
- `ctrl+c` - clear
- `space`  - pause / play

- `up arrow`   - raise fps limit
- `down arrow` - reduce fps limit

- `right shift + up arrow`   - raise particles spawn amount
- `right shift + down arrow` - reduce particles spawn amount

- `left shift + up arrow`   - raise particles spawn radius
- `left shift + down arrow` - reduce particles spawn radius

- `keep pressed left mouse button and drag it to grag field (only with released shift)`
- `slide mouse wheel up or down to zoom in or out`
- `press right mouse button to spawn a bunch of particles under cursor`
- `keep pressed left mouse button with shift to select area`
- `press delete and all selected particles would be removed`
