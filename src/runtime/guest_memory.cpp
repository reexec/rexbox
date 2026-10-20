#include "rexbox/runtime/guest_memory.h"

#include "rexbox/platform/virtual_memory.h"

namespace rexbox::runtime
{

GuestMemory::~GuestMemory()
{
    Shutdown();
}

namespace
{

// Where the RAM shows again with the physical window on, as offsets from
// the base (the generated code adds guest addresses to the base).
constexpr uintptr_t kWindowAliases[] = {GuestMemory::kPhysicalWindow, GuestMemory::kWriteCombinedWindow};
constexpr size_t kWindowAliasCount = sizeof(kWindowAliases) / sizeof(kWindowAliases[0]);

}  // namespace

bool GuestMemory::Initialize(size_t size)
{
    return Initialize(size, Options());
}

bool GuestMemory::Initialize(size_t size, const Options& options)
{
    Shutdown();
    void* block = nullptr;
    if (options.physical_window)
    {
        if (size < kRetailRamSize ||
            !platform::AllocateGuestSpace(kRetailRamSize, size - kRetailRamSize, kWindowAliases, kWindowAliasCount, &block))
        {
            return false;
        }
    }
    else
    {
        block = platform::AllocateVirtualMemory(size, nullptr, false, true);
    }
    if (block == nullptr)
    {
        return false;
    }
    base_ = static_cast<uint8_t*>(block);
    size_ = size;
    physical_window_ = options.physical_window;

    const size_t page_size = platform::PageSize();
    page_shift_ = 0;
    while ((size_t{1} << page_shift_) < page_size)
    {
        ++page_shift_;
    }
    // Tracking works when a query of the first page succeeds; the page is
    // only read here, so the probe loses nothing.
    void* probe_page = nullptr;
    size_t probe_count = 0;
    tracks_writes_ = !physical_window_ && (size_t{1} << page_shift_) == page_size &&
                     platform::TakeWrittenPages(base_, page_size, &probe_page, 1, &probe_count);
    write_version_ = 0;
    page_versions_.assign(tracks_writes_ ? (size + page_size - 1) >> page_shift_ : 0, 0);
    if (probe_count != 0)
    {
        page_versions_[0] = ++write_version_;
    }
    return true;
}

uint64_t GuestMemory::WriteVersion(GuestAddress address, size_t length)
{
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (!tracks_writes_ || length == 0 || !IsValid(address, length))
    {
        return ++write_version_;
    }
    const size_t first = address >> page_shift_;
    const size_t last = (address + length - 1) >> page_shift_;
    const size_t pages = last - first + 1;
    written_pages_.resize(pages);
    size_t found = 0;
    if (!platform::TakeWrittenPages(base_ + (first << page_shift_), pages << page_shift_, written_pages_.data(), pages,
                                    &found))
    {
        found = pages;
        for (size_t i = 0; i < pages; ++i)
        {
            written_pages_[i] = base_ + ((first + i) << page_shift_);
        }
    }
    if (found != 0)
    {
        ++write_version_;
        for (size_t i = 0; i < found; ++i)
        {
            const size_t page = static_cast<size_t>(static_cast<uint8_t*>(written_pages_[i]) - base_) >> page_shift_;
            page_versions_[page] = write_version_;
        }
    }
    uint64_t newest = 0;
    for (size_t page = first; page <= last; ++page)
    {
        newest = page_versions_[page] > newest ? page_versions_[page] : newest;
    }
    return newest;
}

void GuestMemory::Shutdown()
{
    if (base_ != nullptr)
    {
        if (physical_window_)
        {
            platform::FreeGuestSpace(base_, kRetailRamSize, size_ - kRetailRamSize, kWindowAliases, kWindowAliasCount);
        }
        else
        {
            platform::FreeVirtualMemory(base_, size_);
        }
        base_ = nullptr;
        size_ = 0;
    }
    physical_window_ = false;
    tracks_writes_ = false;
    page_versions_.clear();
}

bool GuestMemory::IsValid(GuestAddress address, size_t length) const
{
    if (base_ == nullptr)
    {
        return false;
    }
    if (physical_window_ && address >= kPhysicalWindow)
    {
        // A window range may not run past the RAM it mirrors.
        const GuestAddress folded = Fold(address);
        return folded < kRetailRamSize && length <= kRetailRamSize - folded;
    }
    return address <= size_ && length <= size_ - address;
}

}  // namespace rexbox::runtime
