#include "utils.h"

std::string directory;

std::filesystem::path getSaveDirectory()
{
    // so on MacOS and Linux
    // ~/.config/MYAPP/config.json

    // and on Windows
    // %APPDATA%/(Roaming)/MYAPP/config.json

    std::filesystem::path saveDirectory;
#ifdef _WIN32
    // Windows
    saveDirectory = std::getenv("APPDATA");
    saveDirectory /= "Citrahold";
#else
    // offensively assuming all other platforms are Linux or macOS
    saveDirectory = std::getenv("HOME");
    saveDirectory /= ".config";
    saveDirectory /= "Citrahold";
#endif
    if (!std::filesystem::exists(saveDirectory))
    {
        std::filesystem::create_directories(saveDirectory);
    }

    return saveDirectory;
}

std::filesystem::path getLikelyCitraDirectory()
{
    // so on MacOS 
    // ~/Library/Application Support/Citra/

    // on Linux
    // ~/.local/share/citra-emu/

    // and on Windows
    // AppData/Roaming/Citra/

    std::filesystem::path citraDirectory;
#ifdef _WIN32
    // Windows
    citraDirectory = std::getenv("APPDATA");
    citraDirectory /= "Citra";
#elif __APPLE__
    // macos
    citraDirectory = std::getenv("HOME");
    citraDirectory /= "Library";
    citraDirectory /= "Application Support";
    citraDirectory /= "Citra";
#else
    // offensively assuming all other platforms are Linux
    citraDirectory = std::getenv("HOME");
    citraDirectory /= ".local";
    citraDirectory /= "share";
    citraDirectory /= "citra-emu";
    
#endif
    if (!std::filesystem::exists(citraDirectory))
    {
        return "";
    }

    return citraDirectory;
}