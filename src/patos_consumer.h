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

#ifndef __INCLUDE_PATOS_CONSUMER_H
#define __INCLUDE_PATOS_CONSUMER_H

#include <string>

#include "commandline.h"

#include "clang/AST/Decl.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

class PatosConsumer: public ASTConsumer
{
private:

protected:
    Rewriter *rewriter;
    SourceManager *sourceManager;

    std::string FileName;
    struct Arguments &arguments;

    bool isInSystemFile(Decl *Declaration);

public:
    PatosConsumer(const std::string &FileName, struct Arguments &arguments, Rewriter *rewriter, SourceManager *sourceManager):
        FileName(FileName), arguments(arguments), rewriter(rewriter), sourceManager(sourceManager)
    {
        /* intentionally left blank */
    }

    PatosConsumer(const std::string &FileName, struct Arguments &arguments):
        PatosConsumer(FileName, arguments, NULL, NULL)
    {
        /* intentionally left blank */
    }

    void setRewriter(Rewriter *rewriter)
    {
        this->rewriter = rewriter;
    }

    Rewriter *getRewriter()
    {
        return this->rewriter;
    }

    void setSourceManager(SourceManager *sourceManager)
    {
        this->sourceManager = sourceManager;
    }

    SourceManager *getSourceManager()
    {
        return this->sourceManager;
    }

    void writeChangesToDisk();

    void dumpDeclarationAST(Decl *Declaration, const std::string & subdir);
};

#endif
