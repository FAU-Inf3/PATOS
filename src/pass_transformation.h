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

#ifndef __INCLUDE_PASS_TRANSFORMATION_H
#define __INCLUDE_PASS_TRANSFORMATION_H

#include <string>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <sstream>
#include <set>
#include <map>

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
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Attr.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

using namespace clang;

class PassTransformation:
    public PatosConsumer,
    public RecursiveASTVisitor<PassTransformation>
{
private:
    ASTContext *context;

    std::set<std::string> &templateFiles;

    ClassTemplateDecl *currentClassTemplate;
    Rewriter *currentRewriter;

    int temporaryObjectCounter;
    std::map<Expr *, std::string> temporaryObjectNames;

    std::string expressionToString(const Expr *Expression);
    
    SourceLocation getRealEndLocationForFunctionDeclaration(FunctionDecl *Declaration);

    bool containsMethods(CXXRecordDecl *Declaration);

    std::string createFlatVersionOfRecord(CXXRecordDecl *Declaration);

    void removeDeclarationOrRememberToRemoveInTheFuture(Decl *Declaration);

    SourceRange getSignatureSourceRange(FunctionDecl *Declaration);

    bool isKernelFunction(FunctionDecl *Declaration);

    void transformFunction(FunctionDecl *Declaration, const SourceLocation &insertLocation, bool addDefinitionToMainFile = false);

    bool hasAlreadyADeclaration(const std::string &declarationName);

    // NOTE: this method returns a source range *including* the opening parenthesis of the call expression
    // (possibly followed by some whitespace)
    SourceRange getRealCalleeSourceRange(const CallExpr *Expression);

    SourceRange getDeclaratorSourceRange(FunctionDecl *Declaration);

    bool findUsagesOfTemporaryObjects(Stmt *Statement, std::vector<Expr *> &result);

    std::string getNameForTemporaryObject(Expr *Expression);

    bool hasAlreadyATypeDef(const std::string &recordName);

public:
    PassTransformation(std::string &FileName, struct Arguments &arguments, std::set<std::string> &templateFiles):
        PatosConsumer(FileName, arguments),
        templateFiles(templateFiles),
        currentClassTemplate(NULL),
        currentRewriter(NULL),
        temporaryObjectCounter(0)
    {
        // intentionally left blank
    }

    void HandleTranslationUnit(clang::ASTContext &context);

    bool TraverseClassTemplateDecl(ClassTemplateDecl *Declaration);

    bool TraverseDecl(Decl *Declaration)
    {
        // don't touch system files
        if (!isInSystemFile(Declaration))
        {
            RecursiveASTVisitor::TraverseDecl(Declaration);
        }

        return true;
    }

    bool TraverseCXXRecordDecl(CXXRecordDecl *Declaration);

    bool TraverseCXXMethodDecl(CXXMethodDecl *Declaration);

    bool VisitCXXMethodDecl(CXXMethodDecl *Declaration);

    bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *Declaration);

    bool VisitFunctionDecl(FunctionDecl *Declaration);

    bool TraverseTypeLoc(TypeLoc typeLoc);

    bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *Expression);

    bool VisitCallExpr(CallExpr *Expression);

    bool TraverseCallExpr(CallExpr *Expression);

    bool TraverseCXXMemberCallExpr(CXXMemberCallExpr *Expression);

    bool VisitCXXThisExpr(CXXThisExpr *Expression);

    bool TraverseVarDecl(VarDecl *Declaration);

    bool TraverseCXXConstructExpr(CXXConstructExpr *Expression);

    bool TraverseCompoundStmt(CompoundStmt *Block);

    bool TraverseCXXFunctionalCastExpr(CXXFunctionalCastExpr *Expression);

    bool TraverseCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *Expression);
};

#endif
