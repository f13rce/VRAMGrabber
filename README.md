# VRAMGrabber
Tool that utilizes OpenCL 1.2 and up to fetch the full VRAM of a GPU.

The fetched VRAM is stored in the local home directory (cross-platform), where the size of the raw VRAM data files are identical to the total VRAM available in every individual device.

Tools like Scalpel can be used to analyze the raw VRAM data to retrieve stored data such as images. Check out VRAMFS to do this: https://github.com/Overv/vramfs .

# Requirements
A graphics card or CPU that supports OpenCL 1.2 or newer.

Tested on Ubuntu 18.04, could work on MacOS and Windows 7 and up as well.

# Building
Create a project and link the libOpenCL.so or OpenCL.lib depending on your platform, as well as including the OpenCL 1.2 headers from https://github.com/KhronosGroup/OpenCL-Headers .

Include the main.cpp file and it is ready to be compiled.
