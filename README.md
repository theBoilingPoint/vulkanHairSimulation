# Vulkan Hair Simulation

## Build
This repository only includes the C++ source and header files. Therefore, to actually run the projects on different platforms, you need to link the libraries after cloning following the [Vulkan tutorial](https://vulkan-tutorial.com/Development_environment).

### Windows
You'll need to create an empty project in Visual Studio and add the source and header files to the project. Then, you just need to follow the link provided above.

### MacOS
You'll need to create a new project in Xcode as described in the tutorial and add the source and header files to the project. However, note that the website might be outdated. 
- You should actually instal Vulkan SDK directly from the [official website](https://vulkan.lunarg.com/sdk/home) instead of the link provided in the tutorial.
- If your homebrew doesn't look the same as the one in the tutorial, you should probably add `/opt/homebrew/opt/glfw/include` (non-recursive) to the `Header Search Paths` and `/opt/homebrew/opt/glfw/lib` (non-recursive) to the `Library Search Paths`.