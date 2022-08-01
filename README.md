Quite a bit of Havok-related code comes from [PLANCK](https://github.com/adamhynek/activeragdoll) and [HIGGS](https://github.com/adamhynek/higgs)

The debug drawing functionality is hastily put together from [SmoothCam](https://github.com/mwilsnd/SkyrimSE-SmoothCam)'s code that draws the arrow trajectory. I know it's horribly hacked together but it's just a debug feature, I didn't have time or a real reason to refactor it yet.
 

## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [The Elder Scrolls V: Skyrim Special Edition](https://store.steampowered.com/app/489830)
	* Add the environment variable `CompiledPluginsPath` to point to the folder where you want the .dll to be copied after building
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
	* Desktop development with C++
* [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG/tree/v3.4.0)
	* Add the environment variable `CommonLibSSEPath_NG` with the value as the path to the folder containing CommonLibSSE-NG

## User Requirements
* [Address Library for SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Building
```
git clone https://github.com/ersh1/Precision/
cd Precision
git submodule init
git submodule update

cmake --preset vs2022-windows
cmake --build build --config Release
```
