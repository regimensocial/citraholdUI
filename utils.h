#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <string>

// Function prototypes
std::filesystem::path getSaveDirectory();
std::filesystem::path getLikelyCitraDirectory();

#endif // UTILS_H