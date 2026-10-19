// Paths contract on SDL3.

#include "rexbox/platform/paths.h"

#include <SDL3/SDL.h>

#include <string>

namespace rexbox::platform
{

namespace
{

std::string g_organization;
std::string g_application;

}  // namespace

void SetApplicationIdentity(const std::string& organization, const std::string& application)
{
    g_organization = organization;
    g_application = application;
}

std::string ExecutableDirectory()
{
    const char* base = SDL_GetBasePath();  // owned by SDL
    return base != nullptr ? std::string(base) : std::string();
}

std::string UserSettingsDirectory()
{
    if (g_application.empty())
    {
        return std::string();
    }
    char* pref = SDL_GetPrefPath(g_organization.c_str(), g_application.c_str());
    if (pref == nullptr)
    {
        return std::string();
    }
    std::string path(pref);
    SDL_free(pref);
    return path;
}

}  // namespace rexbox::platform
