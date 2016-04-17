/*
 * Copyright (c) 2016, FAU-Inf3
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __INCLUDE_FILE_HANDLING_H
#define __INCLUDE_FILE_HANDLING_H

#include <vector>
#include <string>

std::string concatPaths(const std::string & path1, const std::string & path2);

std::string getAbsolutePath(const std::string & directory, const std::string & filename, const std::string & extension);

bool findFilesRecursively(const std::string &directoryName, const std::string &suffix, std::vector<std::string> &result);

bool directoryExists(const std::string & path);
bool fileExists(const std::string &fileName);

bool makeDirectories(const std::string & path);

bool copyDirectory(const std::string & source, const std::string & destination);

std::string stripFileName(const std::string & path);

#endif
