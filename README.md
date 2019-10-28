# VRAMGrabber
Tool that utilizes OpenCL 1.2 and up to fetch the full VRAM of all available GPU devices.

The fetched VRAM is stored in the local home directory (cross-platform), where the size of the raw VRAM data files are identical to the total VRAM available in every individual device.

Tools like Scalpel can be used to analyze the raw VRAM data to retrieve stored data such as images. Check out VRAMFS to do this: https://github.com/Overv/vramfs .

# Requirements
A graphics card or CPU that supports OpenCL 1.2 or newer.

Tested on Ubuntu 18.04, could work on MacOS and Windows 7 and up as well.

# Building
Create a project and link the libOpenCL.so or OpenCL.lib depending on your platform, as well as including the OpenCL 1.2 headers from https://github.com/KhronosGroup/OpenCL-Headers .

Include the main.cpp file and it is ready to be compiled.

# Output
After running this file, the data will be stored in your local home directory in the "VRAMDumps" folder. In here you will be able to find a "csv" file and a "raw" file, where the name of the file is your platform + device name.

The CSV file shows how long it took to copy the entire VRAM, where the RAW file is the raw data of the VRAM stored as a binary file. You can use tools like Scalpel or other forensically sound tools to analyze the acquired data.

# About
This project was done by Mike Slotboom and Ivar Slotboom, for class project regarding Computer Crime and Forensics at the University of Amsterdam, where we are studying Security and Network Engineering (SNE, https://os3.nl ).

Research paper of this project can be seen by downloading/viewing the VRAMGrabber-Paper.pdf document.
