# The Dasher Text Entry System
Dasher is a zooming predictive text entry system, designed for situations
where keyboard input is impractical (for instance, accessibility or PDAs). It
is usable with highly limited amounts of physical input while still allowing
high rates of text entry.

## DasherCore
In the past Dasher was developed in a one big repository, featuring multiple frontends with shared code base. This project aims at separating the shared code basis into a _Core_ version, featuring all the functionality needed by all current and future platforms. This allows the "easy" development for new platforms, without having to deal with all other platforms. Currently, we offer Linux (Unix) and Windows support for this library.

If you are looking for a frontend that acutally offers some of the functionality that you might be used to from the old Dasher, have a look at [DasherUI](https://github.com/PapeCoding/DasherUI), which is build based on this library.

## Build Instructions

This library version can be build, simply by generating the required make files via CMake and then building with these. A viable workflow could look something like this:

1. Clone the repository with all submodules: `git clone --recursive https://github.com/dasher-project/DasherCore.git ./DasherCore`
2. Generate some project files with CMake:
  * `cd ./DasherCore && mkdir build && cd build`
  * `cmake ..`
3. Build the project with the selected build system (e.g. Visual Studio on Windows or `make` on Linux)

## License

Dasher was originally built by Inference Group. It was released and maintained under GPL, [you can see that version here.](https://gitlab.gnome.org/GNOME/dasher)
This version of Dasher is licensed under the MIT License.

[You can read more about the relicensing process and why we decided to relicense Dasher here.](./LICENSE_NOTES.md)

## Support and Feedback

Please file any bug reports in the issues of this repository.

You can find the Dasher website and more info at:
https://github.com/dasher-project
