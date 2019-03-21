#include <iostream> // console
#include <CL/cl.hpp>
#include <vector> // buffers
#include <string>
#include <algorithm> // std::replace
#include <fstream> // writing to file
#include <iomanip> // setprecision
#include <sstream> // stringstream

#include <chrono> // time
using namespace std::chrono;

// Directory fetching / creating
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char* outputDirectoryName = "VRAMDumps";

#define MAX_PATH_SIZE 128 // characters
char* GetHomeDirectory()
{
    char homedir[MAX_PATH_SIZE];

#if defined(_WIN32) || defined(_WIN64)
    snprintf(homedir, MAX_PATH_SIZE, "%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
#else
    snprintf(homedir, MAX_PATH_SIZE, "%s", getenv("HOME"));
#endif

    return strdup(homedir);
}

bool DirectoryExists(const std::string& acPath)
{
    if (!acPath.empty())
    {
        if (access(acPath.c_str(), 0) == 0)
        {
            struct stat status;
            stat(acPath.c_str(), &status);
            if (status.st_mode & S_IFDIR) // Check if the directory flag is set
            {
            	return true;
            }
        }
    }

    // if any condition fails
    return false;
}

bool CreateDirectory(const std::string& acPath)
{
	int error = 0;

#if defined(_WIN32) || defined(_WIN64)
	error = _mkdir(acPath.c_str());
#else
	  mode_t nMode = 0733; // UNIX style permissions
	  error = mkdir(acPath.c_str(), nMode);
#endif

	if (error != 0)
	{
		std::cout << "Warning: Failed to create a directory in " << acPath << ". Error code: " << std::to_string(error) << std::endl;
		return false;
	}

	return true;
}

std::string GetSanitizedName(const std::string& acName)
{
	std::string ret = acName;
	std::replace(ret.begin(), ret.end(), ' ', '_');
	return ret;
}

uint64_t GetTimeInMs()
{
	milliseconds ms = duration_cast< milliseconds >(
	    system_clock::now().time_since_epoch()
	);

	return ms.count();
}

void AppendToLogFile(const std::string& acPath, uint64_t aBlockIndex, uint64_t aTime, float aPercentage)
{
	std::ofstream logFile;
	logFile.open(acPath, std::ios_base::app);
	logFile << std::to_string(aBlockIndex) << ", " << std::to_string(aTime) << ", " << std::to_string(aPercentage) << '\n';
	logFile.close();
}

static char kernelSourceCode[] =
"__kernel void                                                               \n"
"donothing()                                                                 \n"
"{                                                                           \n"
"}                                                                           \n"
;

static constexpr uint64_t maxBlockSize = 8192; // 8 KiB

int main()
{
	std::cout << "VRAM Grabber by Ivar & Mike Slotboom" << std::endl << std::endl;

	// Set up output directory
	std::string outputPath = "";
	outputPath += GetHomeDirectory();
	outputPath += "/";
	outputPath += outputDirectoryName;

	if (!DirectoryExists(outputPath))
	{
		std::cout << "Setting up output directory in " << outputPath << std::endl;
		if (!CreateDirectory(outputPath))
		{
			return 3;
		}
		std::cout << "Directory has been created." << std::endl << std::endl;
	}

	// Fetch all platforms that are linked to OpenCL
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	if (platforms.empty())
	{
		std::cout << "Error: No platforms were found." << std::endl;
		return 1;
	}

	// Print found platforms that support OpenCL
	std::cout << "Found " << std::to_string(platforms.size()) << " platform(s):" << std::endl;
	for (size_t i = 0; i < platforms.size(); ++i)
	{
		std::cout << '\t' << std::to_string(i + 1) << ": " << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;
	}

	// Sanitize chat
	std::cout << std::endl;

	for (size_t p = 0; p < platforms.size(); ++p)
	{
		auto& current_platform = platforms[p];
		std::cout << "Processing platform " << std::to_string(p+1) << " of " << std::to_string(platforms.size()) << ": " << current_platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

		std::vector<cl::Device> all_devices;
		current_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices); // Filter by GPU for actual VRAM instead of simulated VRAM

		if (all_devices.empty())
		{
			std::cout << "Error: No devices were found." << std::endl;
			return 2;
		}

		// Print found devices that support OpenCL
		std::cout << "Found " << std::to_string(all_devices.size()) << " device(s) linked to this platform:" << std::endl;
		for (size_t i = 0; i < all_devices.size(); ++i)
		{
			std::cout << '\t' << std::to_string(i + 1) << ": " << all_devices[i].getInfo<CL_DEVICE_NAME>() << std::endl;
		}

		// Sanitize chat
		std::cout << std::endl;

		// Process every found device
		for (size_t i = 0; i < all_devices.size(); ++i)
		{
			auto& current_device = all_devices[i];

			// Device
			std::cout << "Processing device " << std::to_string(i+1) << " of " << std::to_string(all_devices.size()) << ": " << current_device.getInfo<CL_DEVICE_NAME>() << std::endl;
			std::cout << "Device vendor: " << current_device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
			std::cout << "Driver version: " << current_device.getInfo<CL_DRIVER_VERSION>() << std::endl;
			std::cout << "Device version: " << current_device.getInfo<CL_DEVICE_VERSION>() << std::endl;

			// Memory
			std::cout << std::endl << "Memory info:" << std::endl;
			std::cout << "Global memory size: " << current_device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl;
			std::cout << "Local memory size: " << current_device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
			const uint64_t maxAllocSize = current_device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
			std::cout << "Max alloc size: " << maxAllocSize << std::endl; // 523927552 b = 499.65625MiB

			std::cout << std::endl;

			cl::Context context(current_device);

			std::cout << "Preparing buffer..." << std::endl;
			std::vector<cl::Buffer> buffers;

			// Prepare program
			std::cout << "Building program..." << std::endl;
			cl::Program::Sources sources(1, std::make_pair(kernelSourceCode, 0));
			cl::Program program(context, sources);
			program.build(all_devices);

			std::cout << "Setting up kernel instructions..." << std::endl;
			cl::Kernel kernel(program, "donothing");

			std::cout << "Performing copy task..." << std::endl;

			cl::CommandQueue queue(context, current_device, 0);

			// Create file path
			std::string filePath = "";
			filePath += outputPath;
			filePath += "/";
			filePath += GetSanitizedName(current_platform.getInfo<CL_PLATFORM_NAME>()).c_str(); // Needs c_str() to prevent null terminator
			filePath += "_";
			filePath += GetSanitizedName(current_device.getInfo<CL_DEVICE_NAME>()).c_str(); // Needs c_str() to prevent null terminator
			filePath +=  "_id" + std::to_string(i+1);
			filePath += ".raw";

			// Create log file
			std::string logFilePath = "";
			logFilePath += outputPath;
			logFilePath += "/";
			logFilePath += GetSanitizedName(current_platform.getInfo<CL_PLATFORM_NAME>()).c_str(); // Needs c_str() to prevent null terminator
			logFilePath += "_";
			logFilePath += GetSanitizedName(current_device.getInfo<CL_DEVICE_NAME>()).c_str(); // Needs c_str() to prevent null terminator
			logFilePath +=  "_id" + std::to_string(i+1);
			logFilePath += ".csv";

			// Prepare log file
			std::cout << "Saving log file for this device to " << logFilePath << std::endl;
			std::ofstream logFile;
			logFile.open(logFilePath, std::fstream::out | std::fstream::trunc | std::fstream::binary);
			logFile << "Block Index, Time (ms), Percentage" << '\n';
			logFile.close();

			// Start writing to file
			std::cout << "Saving VRAM of this device to " << filePath << std::endl;
			std::ofstream rawDataFile;
			rawDataFile.open(filePath, std::fstream::out | std::fstream::trunc | std::fstream::binary);

			// Iterate over VRAM
			uint64_t writeCount = current_device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / maxBlockSize;

			uint64_t curMs = 0;
			static constexpr uint64_t cLogAndPrintInterval = 1500; // ms
			uint64_t start = GetTimeInMs();

			for (uint64_t i = 0; i < writeCount; ++i)
			{
				// Make temp buffer
				std::string hostBuf;
				hostBuf.resize(maxBlockSize);
				memset(&hostBuf[0], 0, hostBuf.size());
				buffers.push_back(cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR, maxBlockSize));

				// Read VRAM to temp buffer
				queue.enqueueReadBuffer(buffers.back(), CL_TRUE, 0, maxBlockSize, &hostBuf[0]);

				auto now = GetTimeInMs();
				if (now - curMs >= cLogAndPrintInterval)
				{
					curMs = now;
					float pct = float(100.f / float(writeCount) * float(i));
					std::stringstream stream;
					stream << std::fixed << std::setprecision(2) << pct;
					std::cout << "Writing buffer " << (i) << "/" << writeCount << " (" << stream.str() << "%)" << std::endl;
					AppendToLogFile(logFilePath, i, now - start, pct);
				}

				// Write to file
				rawDataFile.write((char*)&hostBuf[0], maxBlockSize);
			}

			printf("Done copying VRAM. Closing file...\n");
			rawDataFile.close();
		}
	}

	// Clean up OpenCL
	cl::finish();

	// Done!
	std::cout << std::endl << "Done copying all devices of all platforms!" << std::endl;

	return 0;
}
