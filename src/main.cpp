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

#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "commandline.h"
#include "driver.h"
#include "file_handling.h"

// Entry point for application
int main(int argc, char **argv)
{
    std::cout << COL_YELLOW;
    std::cout << "---------------------------------" << std::endl;
    std::cout << " Patos source-to-source compiler " << std::endl;
    std::cout << "---------------------------------" << std::endl;
    std::cout << COL_CLEAR << std::endl;

    // parse command line arguments and store information in 'arguments'
    struct Arguments arguments;
    if (!parseCommandLineArguments(argc, argv, arguments))
    {
        return EXIT_FAILURE;
    }

    // check whether specified directories exist
    {
        if (!directoryExists(arguments.InputDirectory))
        {
            ERROR << "input directory does not exist: " << arguments.InputDirectory << std::endl;
            return EXIT_FAILURE;
        }
        if (!directoryExists(arguments.OutputDirectory))
        {
            INFO << "Creating directory for output files: " << arguments.OutputDirectory<< std::endl;

            if (!(makeDirectories(arguments.OutputDirectory)))
            {
                ERROR << "unable to create directories in path '" << arguments.OutputDirectory << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        if (arguments.DumpAST && !directoryExists(arguments.ASTDumpDirectory))
        {
            INFO << "Creating directory for AST dumps: " << arguments.ASTDumpDirectory<< std::endl;

            if (!(makeDirectories(arguments.ASTDumpDirectory)))
            {
                ERROR << "unable to create directories in path '" << arguments.ASTDumpDirectory << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    // print some information to stdout
    {
        INFO << "Using input directory " << arguments.InputDirectory << std::endl;
        INFO << "Using output directory " << arguments.OutputDirectory << std::endl;

        if (arguments.DumpAST)
        {
            INFO << "Dump ASTs to " << arguments.ASTDumpDirectory << std::endl;
        }

        if (arguments.SystemIncludePaths.empty())
        {
            INFO << "No include paths provided" << std::endl;
        }
        else
        {
            INFO << "List of include paths:" << std::endl;
            for (auto it = arguments.SystemIncludePaths.begin(); it != arguments.SystemIncludePaths.end(); ++it)
            {
                INFO << "   "  << *it << std::endl;
            }
        }
    }

    if (arguments.ExplicitInstantiation)
    {
        // get user input for kernel file, kernel name and arguments
        std::string input;
        std::string kernelFile;
        std::string kernelName;
        std::vector<std::string> templateArguments;
        std::vector<std::string> argumentTypes;
        int nrTemplateArguments = -1;
        int nrArguments = -1;

        // kernel file
        {
            INPUT << "name of file containing kernel definition: ";
            std::getline(std::cin, kernelFile);
        }

        // kernel name
        {
            INPUT << "name of kernel to instantiate: ";
            std::getline(std::cin, kernelName);
        }

        // template arguments
        {
            INPUT << "number of template arguments: ";
            std::getline(std::cin, input);
            nrTemplateArguments = std::stoi(input);

            if (nrTemplateArguments < 0)
            {
                ERROR << "invalid number of template arguments" << std::endl;
                return EXIT_FAILURE;
            }

            for (unsigned i = 0; i < nrTemplateArguments; ++i)
            {
                INPUT << "template argument " << (i+1) << ": ";
                std::getline(std::cin, input);
                templateArguments.push_back(input);
            }
        }

        // argument types
        {
            INPUT << "number of argument types: ";
            std::getline(std::cin, input);
            nrArguments = std::stoi(input);

            if (nrArguments < 0)
            {
                ERROR << "invalid number of arguments" << std::endl;
                return EXIT_FAILURE;
            }

            for (unsigned i = 0; i < nrArguments; ++i)
            {
                INPUT << "argument type " << (i+1) << ": ";
                std::getline(std::cin, input);
                argumentTypes.push_back(input);
            }
        }

        // run the actual transformation and add explicit instantiation of kernel template
        instantiateKernel(arguments, kernelFile, kernelName, templateArguments, argumentTypes);
    }
    else
    {
        // run the actual transformation
        runTransformation(arguments);
    }

    return 0;
}
