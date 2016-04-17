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

#include "file_handling.h"

#include "common.h"

#define BOOST_NO_SCOPED_ENUMS
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include "boost/filesystem.hpp"
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#undef BOOST_NO_SCOPED_ENUMS

std::string concatPaths(const std::string & path1, const std::string & path2)
{
    namespace fs = boost::filesystem;

    fs::path p(path1);
    p /= path2;

    return p.native();
}

std::string getAbsolutePath(const std::string & directory, const std::string & filename, const std::string & extension)
{
    namespace fs = boost::filesystem;

    fs::path path(directory);

    path /= (filename + extension);

    return fs::absolute(path).native();
}

PRIVATE void findFilesRecursively_Helper(boost::filesystem::path const & directory, const std::string & prefix, const std::string & suffix, std::vector<std::string> & result)
{
    namespace fs = boost::filesystem;

    for (fs::directory_iterator it(directory); it != fs::directory_iterator(); ++it)
    {
        fs::path current(it->path());

        if (fs::is_directory(current))
        {
            findFilesRecursively_Helper(current, prefix + current.filename().string() + fs::path("/").native(), suffix, result);
        }
        else
        {
            if (current.extension() == suffix)
            {
                // found a file we were looking for
                result.push_back(prefix + current.filename().string());
            }
        }
    }
}

bool findFilesRecursively(const std::string & directoryName, const std::string & suffix, std::vector<std::string> & result)
{
    namespace fs = boost::filesystem;

    fs::path directory(directoryName);

    if (!(fs::exists(directory) && fs::is_directory(directory)))
    {
        ERROR << "directory '" << directoryName << "' does not exist" << std::endl;
        return false;
    }

    findFilesRecursively_Helper(directory, "", suffix, result);
    return true;
}

bool directoryExists(const std::string & path)
{
    namespace fs = boost::filesystem;

    fs::path dirPath(path);

    return fs::exists(dirPath) && fs::is_directory(dirPath);
}

bool fileExists(const std::string &fileName)
{
    namespace fs = boost::filesystem;

    fs::path filePath(fileName);

    return fs::exists(filePath);
}

bool makeDirectories(const std::string & path)
{
    namespace fs = boost::filesystem;

    fs::path directoryPath(path);

    if (fs::exists(directoryPath))
    {
        return fs::is_directory(directoryPath);
    }

    try
    {
        return fs::create_directories(directoryPath);
    }
    catch (fs::filesystem_error const & ex)
    {
        ERROR << ex.what() << std::endl;
        return false;
    }
}

PRIVATE bool copyDirectory_Helper(boost::filesystem::path const & sourcePath, boost::filesystem::path const & destinationPath)
{
    namespace fs = boost::filesystem;

    try
    {
        if (!(makeDirectories(destinationPath.string())))
        {
            return false;
        }

        if (!directoryExists(destinationPath.string()))
        {
        }

        for (fs::directory_iterator it(sourcePath); it != fs::directory_iterator(); ++it)
        {
            fs::path current(it->path());

            if (fs::is_directory(current))
            {
                if (!(copyDirectory_Helper(current, destinationPath / current.filename())))
                {
                    return false;
                }
            }
            else
            {
                fs::copy_file(current, destinationPath / current.filename(), fs::copy_option::overwrite_if_exists);
            }
        }
    }
    catch (fs::filesystem_error const & ex)
    {
        ERROR << ex.what() << std::endl;
        return false;
    }

    return true;
}

bool copyDirectory(const std::string & source, const std::string & destination)
{
    namespace fs = boost::filesystem;

    fs::path sourcePath(source);
    fs::path destinationPath(destination);

    if (!(fs::exists(sourcePath) && fs::is_directory(sourcePath)))
    {
        return false;
    }

    return copyDirectory_Helper(sourcePath, destinationPath);
}

std::string stripFileName(const std::string & path)
{
    namespace fs = boost::filesystem;

    fs::path filePath(path);
    filePath.remove_filename();

    return filePath.string();
}
