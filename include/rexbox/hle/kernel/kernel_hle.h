// HLE module: Xbox kernel (xboxkrnl.exe ordinal exports).
//
// The kernel is the title's only dynamic import, reached through the XBE
// kernel thunk table. Implementations are exported under the names in
// kernel_exports.h. The module needs runtime::Env() installed (memory, heap,
// registry, executor) before InitializeKernel.

#ifndef REXBOX_HLE_KERNEL_KERNEL_HLE_H
#define REXBOX_HLE_KERNEL_KERNEL_HLE_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace rexbox::runtime
{
class GuestMemory;
class HleRegistry;
}

namespace rexbox::xbe
{
class XbeImage;
}

namespace rexbox::hle::kernel
{

void RegisterKernelHle(runtime::HleRegistry& registry);

struct KernelConfig
{
    std::filesystem::path game_directory;  // \Device\CdRom0 (the XBE's folder)
    std::filesystem::path hdd_root;        // holds Partition1..5 of the hard disk
    // The running XBE's file name in game_directory: XeImageFileName reads
    // \Device\CdRom0\<image_file_name>.
    std::string image_file_name = "default.xbe";
};

// Allocates the kernel's data exports in guest memory, mounts the file
// system and starts background services (the tick counter).
void InitializeKernel(const KernelConfig& config);

// The guest address of the kernel data export `name` (XboxKrnlVersion,
// XeImageFileName, ...), or 0 when the HLE has no such export or the kernel
// is not initialized.
uint32_t DataExportAddress(const char* name);

// Points each kernel thunk slot of the placed image at its target: a data
// export's guest address, or the ordinal's synthetic VA
// (rexbox/runtime/kernel_thunks.h) bound to the export of the same name.
// Returns the ordinals with no implementation yet; their slots are written all
// the same. Call after InitializeKernel.
std::vector<uint32_t> BindKernelImports(const xbe::XbeImage& image, runtime::GuestMemory& memory,
                                        runtime::HleRegistry& registry);

// Starts the image's entry point as the first guest thread, with the TLS the
// XBE declares.
bool StartInitialThread(const xbe::XbeImage& image, std::string* error);

}  // namespace rexbox::hle::kernel

#endif  // REXBOX_HLE_KERNEL_KERNEL_HLE_H
