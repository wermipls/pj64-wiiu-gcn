#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define PLUGIN_NAME "pj64-wiiu-gcn"
#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 0
#define PLUGIN_VERSION_PATCH 0
#define PLUGIN_VERSION TOSTRING(PLUGIN_VERSION_MAJOR) "." \
                       TOSTRING(PLUGIN_VERSION_MINOR) "." \
                       TOSTRING(PLUGIN_VERSION_PATCH)

#define PLUGIN_NAMEVER PLUGIN_NAME " v" PLUGIN_VERSION
#define PLUGIN_REPO "https://github.com/wermipls/pj64-wiiu-gcn"
