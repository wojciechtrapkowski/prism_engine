### Setting Up Vulkan Environment Variables

To build and run this project correctly, you need to configure the Vulkan SDK environment variables on your system:

#### **Windows**
1. **Install the Vulkan SDK** from [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home).  
2. **Set the environment variables**:  
   - `VULKAN_SDK` → path to your Vulkan SDK installation (e.g., `C:\VulkanSDK\1.4.321.0`)  
   - Add the Vulkan `bin` directory to your `PATH`:

     ```text
     %VULKAN_SDK%\Bin;%PATH%
     ```
#### **MacOS**