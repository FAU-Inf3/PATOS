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

#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include <sstream>

#include "driver.h"
#include "config.h"
#include "common.h"
#include "file_handling.h"
#include "parse.h"

#include "pass_transformation.h"
#include "pass_remove_templates.h"
#include "pass_sanitize.h"

#include "clang/Basic/SourceManager.h"


//====== DRIVER ======//


PRIVATE void createIncludePaths(std::vector<std::string> &paths, std::vector<IncludePath> &result)
{
    for (auto it = paths.begin(); it != paths.end(); ++it)
    {
        result.push_back(IncludePath(*it, clang::SrcMgr::CharacteristicKind::C_System));
    }
}

PRIVATE void copyInputToOutput(const struct Arguments &arguments)
{
    if (!(copyDirectory(arguments.InputDirectory, arguments.OutputDirectory)))
    {
        ERROR << "unable to copy content of input directory to output directory" << std::endl;
        exit(EXIT_FAILURE);
    }
}

PRIVATE void gatherInputFiles(const struct Arguments &arguments, std::vector<std::string> &result)
{
    if (!(findFilesRecursively(arguments.OutputDirectory, ".m", result)))
    {
        exit(EXIT_FAILURE);
    }
}

PRIVATE void passTransformation(struct Arguments &arguments,
                                std::string &fileName,
                                std::vector<IncludePath> &includePaths,
                                std::set<std::string> &templateFiles)
{
    // get the absolute path for the current file
    std::string absolutePath = getAbsolutePath(arguments.OutputDirectory, fileName, "");

    // create a consumer/pass for the current file
    PassTransformation passTransformation(fileName, arguments, templateFiles);

    // parse the current file
    parseAndConsume(absolutePath, passTransformation, includePaths, true, false);
}

PRIVATE void passRemoveTemplates(struct Arguments &arguments,
                                 const std::string &fileName,
                                 std::vector<IncludePath> &includePaths)
{
    // create a consumer/pass for the current file
    PassRemoveTemplates passRemoveTemplates(fileName, arguments);

    // parse the current file
    parseAndConsume(fileName, passRemoveTemplates, includePaths, true, false);
}

PRIVATE std::string appendExplicitInstantiation(const std::string &fileName,
                                         const std::string &kernelName,
                                         std::vector<std::string> &templateArguments,
                                         std::vector<std::string> &argumentTypes)
{
    // create source for explicit specialization
    std::stringstream strInstantiation;
    {
        strInstantiation << "template __kernel void ";

        // kernel name
        strInstantiation << kernelName;

        strInstantiation << "<";

        // template arguments
        for (auto it = templateArguments.begin(); it != templateArguments.end(); ++it)
        {
            strInstantiation << *it;

            if (std::next(it) != templateArguments.end())
            {
                strInstantiation << ",";
            }
        }

        strInstantiation << " >(";

        // argument types
        for (auto it = argumentTypes.begin(); it != argumentTypes.end(); ++it)
        {
            strInstantiation << *it;

            if (std::next(it) != argumentTypes.end())
            {
                strInstantiation << ",";
            }
        }

        strInstantiation << ");";

        DBG << "explicit instantiation: " << strInstantiation.str() << std::endl;
    }

    std::ofstream kernelFile(fileName, std::ofstream::out | std::ofstream::app);
    kernelFile << strInstantiation.str() << std::endl;
    kernelFile.close();

    return strInstantiation.str();
}

PRIVATE void removeExplicitInstantiation(const std::string &fileName, const std::string &explicitInstantiation)
{
    std::vector<std::string> lines;

    // read lines from kernel file
    {
        std::string line;
        std::ifstream kernelFile(fileName, std::ifstream::in);

        while (std::getline(kernelFile, line))
        {
            if (line.find(explicitInstantiation) == std::string::npos)
            {
                lines.push_back(line);
            }
        }

        kernelFile.close();
    }

    // write filtered lines to file again
    {
        std::ofstream kernelFile(fileName, std::ofstream::out);

        for (auto it = lines.begin(); it != lines.end(); ++it)
        {
            if (it != lines.begin())
            {
                kernelFile << std::endl;
            }

            kernelFile << *it;
        }

        kernelFile.close();
    }
}

void runTransformation(struct Arguments &arguments)
{
    // create list of include paths
    std::vector<IncludePath> includePaths;
    createIncludePaths(arguments.SystemIncludePaths, includePaths);

    // copy content of input directory to output directory
    // this is neccessary, because we only want to work on copies
    copyInputToOutput(arguments);

    // get all input files (files ending with .m) in the output directory
    std::vector<std::string> files;
    gatherInputFiles(arguments, files);

    // some debugging output
    #ifdef DEBUG_MODE
    {
        for (auto it = files.begin(); it != files.end(); ++it)
        {
            DBG << "found input file: " << *it << std::endl;
            DBG << "   absolute: " << getAbsolutePath(*it, "", "") << std::endl;
        }
    }
    #endif

    // this set will contain all files that contain template declarations
    // which have not beed removed yet
    // we have to parse these files again and remove the template declarations
    std::set<std::string> templateFiles;

    // iterate over all input files and parse them
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        passTransformation(arguments, *it, includePaths, templateFiles);
    }

    // now we have to iterate over all files that contain template declarations
    // which have not been removed yet
    for (auto it = templateFiles.begin(); it != templateFiles.end(); ++it)
    {
        // NOTE: filenames in templateFiles are already absolute paths
        passRemoveTemplates(arguments, *it, includePaths);
    }

    // debugging...
    #ifdef PASS_SANITIZE
    {
        // sanitize result by parsing each file again
        // (helpful for finding bugs/inconsistencies in the transformation)
        for (auto it = files.begin(); it != files.end(); ++it)
        {
            // get the absolute path for the current file
            std::string absolutePath = getAbsolutePath(arguments.OutputDirectory, *it, "");

            // create a consumer/pass for the current file
            PassSanitize passSanitize(*it, arguments);

            // parse the current file
            parseAndConsume(absolutePath, passSanitize, includePaths, false, true);
        }
    }
    #endif
}

std::string instantiateKernel(
                    struct Arguments &arguments,
                    const std::string &kernelFile,
                    const std::string &kernelName,
                    std::vector<std::string> &templateArguments,
                    std::vector<std::string> &argumentTypes
                    )
{
    // create list of include paths
    std::vector<IncludePath> includePaths;
    createIncludePaths(arguments.SystemIncludePaths, includePaths);

    // copy content of input directory to output directory
    // this is neccessary, because we only want to work on copies
    copyInputToOutput(arguments);

    // check if kernel file exists
    std::string kernelFileAbsolute = getAbsolutePath(arguments.OutputDirectory, kernelFile, "");
    if (!fileExists(kernelFileAbsolute))
    {
        ERROR << "kernel file does not exist" << std::endl;
        exit(EXIT_FAILURE);
    }

    // append explicit instantiation of kernel to input file
    std::string explicitInstantiation = appendExplicitInstantiation(kernelFileAbsolute, kernelName, templateArguments, argumentTypes);

    // get all input files (files ending with .m) in the output directory
    std::vector<std::string> files;
    gatherInputFiles(arguments, files);

    // this set will contain all files that contain template declarations
    // which have not beed removed yet
    // we have to parse these files again and remove the template declarations
    std::set<std::string> templateFiles;

    // iterate over all input files and parse them
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        passTransformation(arguments, *it, includePaths, templateFiles);
    }

    // now we have to iterate over all files that contain template declarations
    // which have not been removed yet
    for (auto it = templateFiles.begin(); it != templateFiles.end(); ++it)
    {
        // NOTE: filenames in templateFiles are already absolute paths
        passRemoveTemplates(arguments, *it, includePaths);
    }

    // remove explicit instantiation from kernel file
    removeExplicitInstantiation(kernelFileAbsolute, explicitInstantiation);

    // now we can get the (type-mangled) name of the kernel instantiation
    std::string mangledName = PatosNameMangling::getMangledNameForKernel(kernelName, templateArguments);
    return mangledName;
}
