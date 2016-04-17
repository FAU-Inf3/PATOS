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

#ifndef __INCLUDE_COMMANDLINE_H
#define __INCLUDE_COMMANDLINE_H

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

struct Arguments
{
    std::string InputDirectory;
    std::string OutputDirectory;

    bool DumpAST;
    std::string ASTDumpDirectory;

    std::vector<std::string> SystemIncludePaths;

    bool ExplicitInstantiation;
};

/**
 * Function for parsing the command line arguments provided by the user.
 * Saves the parsed information in an Arguments instance and returns whether
 * command line arguments were valid.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @param arguments Reference to an Arguments instance
 *        where the parsed information should be stored
 *
 * @return True, if command line arguments were valid, false otherwise.
 */
bool parseCommandLineArguments(int argc, char **argv, struct Arguments &arguments);

#endif
