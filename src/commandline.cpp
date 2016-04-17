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

#include <string>
#include <vector>
#include <stdlib.h>

#include "commandline.h"
#include "common.h"

#include "boost/program_options.hpp"

bool parseCommandLineArguments(int argc, char **argv, struct Arguments &arguments)
{
    namespace po = boost::program_options;

    po::options_description description("arguments for the Patos source-to-source compiler");
    description.add_options()
        ("help,h", "print help message")
        ("input-dir,i", po::value<std::string>(&arguments.InputDirectory)->required(), "set input directory")
        ("output-dir,o", po::value<std::string>(&arguments.OutputDirectory)->required(), "set output directory")
        ("astdump-dir,d", po::value<std::string>(&arguments.ASTDumpDirectory), "set directory to dump the ASTs to")
        ("include-path,I", po::value<std::vector<std::string>>(&arguments.SystemIncludePaths)->composing(), "add path to list of include paths")
        ("explicit-instantiation,e", po::bool_switch(&arguments.ExplicitInstantiation)->default_value(false), "ask for explicit instantiation of kernel function");

    po::variables_map var_map;
    try
    {
        po::store(boost::program_options::parse_command_line(argc, argv, description), var_map);

        if (var_map.count("help"))
        {
            std::cout << description << std::endl;
            exit(EXIT_SUCCESS);
        }

        po::notify(var_map);
    }
    catch (po::error const & ex)
    {
        ERROR << ex.what() << std::endl;
        return false;
    }

    arguments.DumpAST = (var_map.count("astdump-dir") > 0);

    return true;
}
