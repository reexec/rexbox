// Platform contract: folders the application keeps files in.

#ifndef REXBOX_PLATFORM_PATHS_H
#define REXBOX_PLATFORM_PATHS_H

#include <string>

namespace rexbox::platform
{

// The folder of the running executable, ending in a separator; empty if
// unknown.
std::string ExecutableDirectory();

// Names the application for the per-user settings folder (an organization
// and an application name, as the OS conventions use them). Set by the
// application before UserSettingsDirectory is asked; rexbox names no title.
void SetApplicationIdentity(const std::string& organization, const std::string& application);

// A per-user folder for the application's settings (created if needed),
// ending in a separator; empty if unavailable or no identity was set.
std::string UserSettingsDirectory();

}  // namespace rexbox::platform

#endif  // REXBOX_PLATFORM_PATHS_H
