#pragma once

#include "common/utils.h"

#define PROJECT_NAME                "Cube-Romin"
#define VERSION_MAJOR               0  // increment when a major release is made (big new feature, etc)
#define VERSION_MINOR               1  // increment when a minor release is made (small new feature, change etc)
#define VERSION_PATCH_LEVEL         0  // increment when a bug is fixed

#define VERSION_STRING STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH_LEVEL)

extern const char* const targetName;

#define GIT_SHORT_REVISION_LENGTH   7 // lower case hexadecimal digits.
extern const char* const shortGitRevision;

#define BUILD_DATE_LENGTH           11
extern const char* const buildDate;  // "MMM DD YYYY" MMM = Jan/Feb/...

#define BUILD_TIME_LENGTH           8
extern const char* const buildTime;  // "HH:MM:SS"
