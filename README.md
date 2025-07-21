# AO Communication APP - Mockup

This module mimics the communicates with the API of the [Neuro Omega](https://www.alphaomega-eng.com/Neuro-Omega-System) machine. Running the application, will spawn up a TCP server that processes the following commands:

- `start`
- `stop`

## Build

We are now using [meson](https://mesonbuild.com/) approach for setting up the build of this app.

- Check the `meson.build` file and adjust for your machine
- Create the build dir with `meson build`
- Build using `meson compile -C build`

## Run

- Run the app using `./build/ao_comm`

#### The Dareplane Overview

[Back to the Dareplane landing page](https://github.com/bsdlab/Dareplane)

#### Trouble shooting

- App compiles but does not start and not print anything: This can be the case if you do not have the `lsl.dll` in the main directory! Just linking against the static library is not enough!
