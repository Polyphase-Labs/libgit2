# Compiling (Visual Studio Code)

1. Open the root polyphase folder in VsCode (where this README is located).
2. Install the C/C++ Extension Pack.
3. Run the Polyphase Editor config (click the Run and Debug tab on the left, then in the drop down where the green Play button is, select `Polyphase Editor`).


# Compiling (Terminal)

1. From the project's root directory (where this README is located) `cd Standalone`
2. Run `make -f Makefile_Linux_Editor`
3. Go back to the root directory `cd ..`
4. Run `Standalone/Build/Linux/PolyphaseEditor.elf` It's important that the working directory is the root directory where the Engine and Standalone folders are located.

## Packaging

1. When packaging for any platform on a Windows machine, you will likely need to install Msys2 so that linux commands can be executed. This comes packaged along with devkitPro libraries, so you if you install the devkitPro libraries, you shouldn't need to worry about this.
2. Packaging for Android requires installing Android Studio (Last tested with Android Studio 2022.2.1 Patch 2) with the following tools installed via the SDK Manager:
   - Android SDK Build Tools: 34.0.0
   - Android NDK (Side by side): 25.2.9519653
   - CMake: 3.22.1

## CMake Support

CMake support is currently a work-in-progress, and only Linux support has been implemented and tested. If you want to try building with CMake, here are some tips:

- Make sure you pull all submodules `git submodule update --init --recursive`
- Install pkg-config `sudo apt install pkg-config`(debian/ubuntu), `
- Install vorbis dev libraries `sudo apt install libvorbis-dev`
