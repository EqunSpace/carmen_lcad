
#ifndef __SEGMAP_UTIL_IO_H__
#define __SEGMAP_UTIL_IO_H__

#include <cstdio>
#include <string>
#include <vector>

FILE* safe_fopen(const char *path, const char *mode);

std::vector<double> read_vector(char *name);
std::string file_name_from_path(const char *path);

#endif
