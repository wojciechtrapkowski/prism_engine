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
      