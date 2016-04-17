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

#ifndef __INCLUDE_PASS_REMOVE_TEMPLATES_H
#define __INCLUDE_PASS_REMOVE_TEMPLATES_H

#include <string>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <sstream>

#include "commandline.h"
#include "common.h"
#include "patos_consumer.h"
#include "file_handling.h"
#include "name_mangling.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"

using namespace clang;

class PassRemoveTemplates:
    public PatosConsumer,
    public RecursiveASTVisitor<PassRemoveTemplates>
{
private:
    ASTContext *context;

    bool containsMethods(CXXRecordDecl *Declaration)
    {
        // check if there are template methods
        for (auto it = Declaration->decls_begin(); it != Declaration->decls_end(); ++it)
        {
            Decl *declaration = *it;

            if (isa<CXXConstructorDecl>(declaration) && !Declaration->hasUserDeclaredConstructor())
            {
                continue;
            }

            if (isa<FunctionTemplateDecl>(declaration) || isa<CXXMethodDecl>(declaration))
            {
                return true;
            }
        }

        return false;
    }

    SourceLocation getRealEndLocation(Decl *Declaration)
    {
        SourceManager &sourceManager = this->context->getSourceManager();
        const LangOptions &languageOptions = this->context->getLangOpts();

        SourceLocation locationEnd(Lexer::getLocForEndOfToken(Declaration->getLocEnd(), 0, sourceManager, languageOptions));
        SourceLocation locationSemi = Lexer::findLocationAfterToken(Declaration->getLocEnd(), tok::semi, sourceManager, languageOptions, true);

        if (locationSemi.isValid())
        {
            locationEnd = locationSemi;
        }

        return locationEnd;
    }

public:
    PassRemoveTemplates(const std::string &FileName, struct Arguments &arguments):
        PatosConsumer(FileName, arguments)
    {
        // intentionally left blank
    }

    void HandleTranslationUnit(clang::ASTContext &context)
    {
        DBG << "Consume [REMOVE TEMPLATES]: " << this->FileName << std::endl;

        // TODO absolute path...
        //if (this->arguments.DumpAST)
        //{
        //    this->dumpDeclarationAST(context.getTranslationUnitDecl(), "remove_templates");
        //}

        this->context = &context;
        this->TraverseDecl(context.getTranslationUnitDecl());

        // write result to disk
        this->writeChangesToDisk();
    }

    bool TraverseClassTemplateDecl(ClassTemplateDecl *Declaration)
    {
        DBG << "remove class template declaration: " << Declaration->getNameAsString() << std::endl;

        // remove template declaration
        this->rewriter->RemoveText(SourceRange(Declaration->getLocStart(), this->getRealEndLocation(Declaration)));

        // NOTE: we do not have to traverse the declaration's child nodes, since we removed the declaration

        return true;
    }

    bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *Declaration)
    {
        DBG << "remove function template declaration: " << Declaration->getNameAsString() << std::endl;

        // remove template declaration
        this->rewriter->RemoveText(SourceRange(Declaration->getLocStart(), this->getRealEndLocation(Declaration)));

        // NOTE: in this case, we have to traverse the declaration recursively (albeit having removed it),
        // since it might contain a method declaration
        RecursiveASTVisitor::TraverseFunctionTemplateDecl(Declaration);

        return true;
    }

    bool TraverseCXXRecordDecl(CXXRecordDecl *Declaration)
    {
        if (this->containsMethods(Declaration))
        {
            DBG << "remove record declaration containing methods: " << Declaration->getNameAsString() << std::endl;

            // remove template declaration
            this->rewriter->RemoveText(SourceRange(Declaration->getLocStart(), this->getRealEndLocation(Declaration)));

            // NOTE: we do not have to traverse the declaration's child nodes, since we removed the declaration

            return true;
        }

        RecursiveASTVisitor::TraverseCXXRecordDecl(Declaration);
        return true;
    }

    bool TraverseCXXMethodDecl(CXXMethodDecl *Declaration)
    {
        // NOTE: after the transformation, no more method declarations are needed

        DBG << "remove method declaration: " << Declaration->getNameAsString() << std::endl;

        // remove template declaration
        this->rewriter->RemoveText(SourceRange(Declaration->getLocStart(), this->getRealEndLocation(Declaration)));

        // NOTE: we do not have to traverse the declaration's child nodes, since we removed the declaration

        return true;
    }

};

#endif
