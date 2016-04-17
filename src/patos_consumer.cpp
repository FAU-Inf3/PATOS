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

#include "patos_consumer.h"

#include "common.h"
#include "file_handling.h"

#define SUFFIX_AST_DUMP ".dump"

void PatosConsumer::writeChangesToDisk()
{
    // "Returns true if any files were not saved successfully..."
    if (this->rewriter->overwriteChangedFiles())
    {
        ERROR << "unable to write changes to disk" << std::endl;
        exit(EXIT_FAILURE);
    }
}

bool PatosConsumer::isInSystemFile(Decl *Declaration)
{
    SrcMgr::CharacteristicKind kind = this->sourceManager->getFileCharacteristic(Declaration->getLocStart());
    return kind == SrcMgr::C_System;
}

void PatosConsumer::dumpDeclarationAST(Decl *Declaration, const std::string & subdir)
{
    if (this->arguments.DumpAST)
    {
        std::string fileName = getAbsolutePath(concatPaths(this->arguments.ASTDumpDirectory, subdir), this->FileName, SUFFIX_AST_DUMP);

        if (!(makeDirectories(stripFileName(fileName))))
        {
            ERROR << "unable to create directories for AST dump" << std::endl;
            return;
        }

        DBG << "dumping AST to " << fileName << std::endl;

        // dump AST
        std::string error_info;
        llvm::raw_fd_ostream ostr(fileName.c_str(), error_info, llvm::sys::fs::F_RW);
        Declaration->dump(ostr);
    }
}
