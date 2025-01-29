#pragma once

#include "types.hxx"
using namespace beastie;

#include <cerrno>
#include <cstring>

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace beastie {

// Read a file (in its entirety) into a buffer
template<class T = std::string>
T slurp(std::filesystem::path path);

// Read lines into a buffer
std::vector<std::string> slurpLines(std::filesystem::path path);

// Read a file (in its entirety) into an ull.
unsigned long long slurpULL(std::filesystem::path path);

// Debugging tool...
void printBuffer(std::span<char>, std::string_view name);

// Useful for aligning data...
constexpr auto roundup(std::integral auto i, std::integral auto n)
{
    return (i/n + 1) * n;
}
//static_const(roundup(0,0) == 0);

// Returns platform EFI support
bool isEFI();

// Clean shutdown of the system, init will kexec for us
void shutdown();

// Forced shutdown of the system, kexec now
void forcedshutdown();

// Returns RSDP and RSDT
std::pair<uintptr_t,uintptr_t> fetchACPI20(bool efi);

// Returns framebuffer info
fbinfo fetchFB();

// Returns system map info
smapinfo fetchSMAP(bool debug = true);

// Returns system map info
efimapinfo fetchEFIMAP();

} // namespace beastie
