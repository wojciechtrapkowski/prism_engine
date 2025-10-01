# Prism Engine

First version of the engine was written using OpenGL. However I decided to move to Vulkan due to two reasons. Developing on macOS is very limited - currently it supports only up to version 4.1 of OpenGL. Second reason is that I like to have more control over what is happening, therefore Vulkan suits my needs more.

# Setting Up Vulkan Environment Variables

To build and run this project correctly, you need to install the Vulkan SDK and configure the environment variables on your system.

First, **install the Vulkan SDK** from [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home).

---

## Windows

1. **Set the environment variables**:

   - `VULKAN_SDK` → path to your Vulkan SDK installation, e.g.:

     ```text
     C:\VulkanSDK\1.4.321.0
     ```

   - Add the Vulkan `bin` directory to your `PATH`:

     ```text
      C:\VulkanSDK\1.4.321.0\bin
     ```
  

2. **Restart your terminal** to apply the changes.
---

## macOS

1. **Set the environment variables** in your shell configuration file (`~/.zshrc` or `~/.bash_profile`):

   ```**bash**
   export VULKAN_SDK=PATH_TO_VULKAN_SDK
   export DYLD_LIBRARY_PATH=$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH
   export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
   export VK_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d
   export PATH=$VULKAN_SDK/bin:$PATH
   ```
2. Apply the changes:
    ```**bash**
    source ~/.zshrc   # or source ~/.bash_profile
    ```

# Building the project

## Windows

On Windows the project includes a `setup.bat` script that prepares a `build` folder and generates a Visual Studio solution.

1. Open PowerShell or Command Prompt.
2. Change into the project root if necessary:
   ```powershell
   cd path\to\project
   ```
3. Run the setup script:
   ```powershell
   .\setup.bat
   ```
   - The script will generate a `build` directory and create a `.sln` file (Visual Studio solution) inside `build`.
   - Wait for the script to finish. If it fails, check the console output for missing tools or permissions.

4. Open the solution:
   - In File Explorer, navigate to `build\` and double-click the generated `*.sln` file.
   - Or from the command line:
     ```powershell
     start build\YourSolution.sln
     ```
5. Build and run inside Visual Studio:
   - Select the configuration (Debug/Release) and platform (x86/x64) as appropriate.
   - Build the solution (Build → Build Solution) and run (Debug → Start Debugging or Start Without Debugging).

Notes:
- If you need a clean rebuild, delete the `build` folder and re-run `setup.bat`.

---

## macOS

On macOS the repository provides `build.sh` and `run.sh`.

1. Open Terminal and change to the project root:
   ```bash
   cd /path/to/project
   ```
2. Make sure the scripts are executable (first time only):
   ```bash
   chmod +x ./build.sh ./run.sh
   ```
3. Build:
   - Normal build:
     ```bash
     ./build.sh
     ```
   - Clean:
     ```bash
     ./build.sh --clean
     ```
     The `--clean` flag performs a full clean.
  - Release build:
     ```bash
     ./build.sh --release
     ```
     The `--release` flag instructs the build script to produce optimized/release artifacts (instead of debug builds).
4. Run:
   ```bash
   ./run.sh
   ```
   ```bash
   ./run.sh --release
   ```
   `run.sh` will execute the produced binary (or start the app) using the artifacts in the `build` directory. Pass `--release` to run the artifacts produced by a `--release` build.

---
