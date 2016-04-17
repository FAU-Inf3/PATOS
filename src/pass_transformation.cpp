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

#include "pass_transformation.h"

#define PATOS_KERNEL_ANNOTATION "__patos__kernel"

// ================================================ //
// ===== PASS_TRANSFORMATION: PRIVATE METHODS ===== //
// ================================================ //

std::string PassTransformation::expressionToString(const Expr *Expression)
{
    // try to get source from rewriter to capture any changes made to the expression
    std::string result = this->currentRewriter->getRewrittenText(Expression->getSourceRange());

    if (!result.empty())
    {
        return result;
    }
    else
    {
        // result is empty -> get string from original source

        SourceManager &sourceManager = this->context->getSourceManager();
        const LangOptions &languageOptions = this->context->getLangOpts();

        SourceLocation locationBegin(Expression->getLocStart());
        SourceLocation locationEnd(Lexer::getLocForEndOfToken(Expression->getLocEnd(), 0, sourceManager, languageOptions));

        return Lexer::getSourceText(CharSourceRange::getCharRange(locationBegin, locationEnd), sourceManager, languageOptions);
    }
}

SourceLocation PassTransformation::getRealEndLocationForFunctionDeclaration(FunctionDecl *Declaration)
{
    SourceManager &sourceManager = this->context->getSourceManager();
    const LangOptions &languageOptions = this->context->getLangOpts();

    SourceLocation locationEnd(Lexer::getLocForEndOfToken(Declaration->getLocEnd(), 0, sourceManager, languageOptions));

    // if declaration is just a forward declaration, we have to change the end location to include the ';'
    // (don't know why...)
    if (!Declaration->isThisDeclarationADefinition())
    {
        // forward declaration
        locationEnd = Lexer::findLocationAfterToken(Declaration->getLocEnd(), tok::semi, sourceManager, languageOptions, true);

        // assert valid source location
        if (locationEnd.isInvalid())
        {
            ERROR << "invalid forward declaration of function '" << Declaration->getNameAsString() << "'" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return locationEnd;
}

bool PassTransformation::containsMethods(CXXRecordDecl *Declaration)
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

std::string PassTransformation::createFlatVersionOfRecord(CXXRecordDecl *Declaration)
{
    // save old state
    Rewriter *oldRewriter = this->currentRewriter;

    // create a rewriter which we can use to transform the record
    Rewriter recordTransformationRewriter;
    recordTransformationRewriter.setSourceMgr(this->context->getSourceManager(), this->context->getLangOpts());
    this->currentRewriter = &recordTransformationRewriter;

    std::stringstream strstr;

    strstr << "\n" << "typedef struct ";

    // name mangling for record name, if it is a template specialization
    std::string structName;
    if (isa<ClassTemplateSpecializationDecl>(Declaration))
    {
        // perfom name mangling
        structName = PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(Declaration));
    }
    else
    {
        // not a template class -> use original name
        structName = Declaration->getNameAsString();
    }

    strstr << structName << "\n{\n";

    // iterate over all declarations to find the ones which have to be deleted
    // NOTE: first declaration in iterator is record declaration itself -> advance to first member
    for (auto declIt = ++Declaration->decls_begin(); declIt != Declaration->decls_end(); ++declIt)
    {
        Decl *currentDeclaration = *declIt;

        if (isa<FieldDecl>(currentDeclaration))
        {
            // traverse declaration to perform type substitution, ...
            TraverseFieldDecl(cast<FieldDecl>(currentDeclaration));

            // get rewritten source...
            std::string rewrittenText = this->currentRewriter->getRewrittenText(currentDeclaration->getSourceRange());

            // ...and add it to the list of members for the new record
            strstr << "\t" << rewrittenText << ";\n";
        }
    }

    strstr << "} " << structName << ";\n";

    // restore old state
    this->currentRewriter = oldRewriter;

    return strstr.str();
}

void PassTransformation::removeDeclarationOrRememberToRemoveInTheFuture(Decl *Declaration)
{
    SourceLocation locationDeclaration = Declaration->getLocStart();
    SourceManager &sourceManager = this->context->getSourceManager();
    if (sourceManager.isInMainFile(locationDeclaration))
    {
        // is in main file -> remove it directly
        this->rewriter->RemoveText(Declaration->getSourceRange());
    }
    else
    {
        // not in main file -> remember file
        std::string filenameIncluded = sourceManager.getFilename(locationDeclaration).str();
        this->templateFiles.insert(filenameIncluded);

        DBG << "found template declaration in included file: " << std::endl;
        DBG << "   " << filenameIncluded << std::endl;
    }
}

SourceRange PassTransformation::getSignatureSourceRange(FunctionDecl *Declaration)
{
    SourceLocation locationBegin;
   
    // check if this is a constructor
    if (isa<CXXConstructorDecl>(Declaration))
    {
        // constructor
        // -> signature begins with the declarator (no return type)
        locationBegin = Declaration->getNameInfo().getLocStart();
    }
    else
    {
        // not a constructor
        // -> the signature begins with the beginning of the return type
        locationBegin = Declaration->getReturnTypeSourceRange().getBegin();
    }

    // the signature ends with the closing parenthesis ')'
    SourceLocation locationEnd;
    // unfortunately, we have to find this location manually
    {
        SourceManager &sourceManager = this->context->getSourceManager();
        const LangOptions &languageOptions = this->context->getLangOpts();

        if (Declaration->getNumParams() > 0)
        {
            // has parameters
            // -> get end of last parameters
            locationEnd = Declaration->getParamDecl(Declaration->getNumParams()-1)->getLocEnd();
            locationEnd = Lexer::getLocForEndOfToken(locationEnd, 0, sourceManager, languageOptions);
        }
        else
        {
            // has no parameters
            // -> find position of opening parenthesis '(' starting at the end of the declarator
            locationEnd = Declaration->getNameInfo().getLocEnd();
            locationEnd = Lexer::findLocationAfterToken(locationEnd, tok::l_paren, sourceManager, languageOptions, true);
        }
    }

    return SourceRange(locationBegin, locationEnd);
}

bool PassTransformation::isKernelFunction(FunctionDecl *Declaration)
{
    for (auto it = Declaration->attr_begin(); it != Declaration->attr_end(); ++it)
    {
        Attr *attribute = *it;

        // check if this attribute represents an annotation of "__patos__kernel"
        if (isa<AnnotateAttr>(attribute) && cast<AnnotateAttr>(attribute)->getAnnotation().str() == PATOS_KERNEL_ANNOTATION)
            return true;
    }

    return false;
}

void PassTransformation::transformFunction(FunctionDecl *Declaration, const SourceLocation &insertLocation, bool addDefinitionToMainFile)
{
    // traverse specialization to perform type substitution, name mangling, call replacement, etc.
    if (isa<CXXMethodDecl>(Declaration))
    {
        TraverseCXXMethodDecl(cast<CXXMethodDecl>(Declaration));
    }
    else
    {
        TraverseFunctionDecl(Declaration);
    }

    // add transformed version to rewriter
    {
        // unfortunately, the function signature's locations point to the declaration,
        // whereas the locations for the function's body point to the definition
        // -> gather rewritten code of signature and body from different locations

        std::stringstream strRewrittenText;
        strRewrittenText << "\n";

        // if function is a kernel function, we have to add a '__kernel' before it
        if (isKernelFunction(Declaration))
        {
            strRewrittenText << "__kernel ";
        }

        // if function is a constructor, we have to add the return type before it
        if (isa<CXXConstructorDecl>(Declaration))
        {
            CXXConstructorDecl *constructorDeclaration = cast<CXXConstructorDecl>(Declaration);
            CXXRecordDecl *parent = constructorDeclaration->getParent();

            // get name of parent
            std::string parentName;
            if (isa<ClassTemplateSpecializationDecl>(parent))
            {
                // parent is a class template specialization
                // -> get mangled name
                parentName = PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(parent));
            }
            else
            {
                // parent is a 'normal' record
                parentName = parent->getNameAsString();
            }

            // add return type
            strRewrittenText << "struct " << parentName << " ";
        }

        // rewritten code of signature
        strRewrittenText << this->currentRewriter->getRewrittenText(getSignatureSourceRange(Declaration));

        // add declaration to source
        std::string declarationSource = strRewrittenText.str() + ";\n";
        this->rewriter->InsertTextAfter(insertLocation, declarationSource);

        // rewritten code of body (if any)
        if (addDefinitionToMainFile && Declaration->hasBody())
        {
            strRewrittenText << "\n" << this->currentRewriter->getRewrittenText(Declaration->getBody()->getSourceRange()) << "\n";

            // this is a definition
            // definitions have to be added to the _module_, i.e. we have to insert it in the main file
            SourceLocation locationModule = this->context->getSourceManager().getLocForEndOfFile(this->context->getSourceManager().getMainFileID());
            this->rewriter->InsertTextAfter(locationModule, strRewrittenText.str());
        }
    }
}

bool PassTransformation::hasAlreadyADeclaration(const std::string &declarationName)
{
    // iterate over all declarations in the AST
    TranslationUnitDecl *translationUnit = this->context->getTranslationUnitDecl();
    for (auto it = translationUnit->decls_begin(); it != translationUnit->decls_end(); ++it)
    {
        Decl *otherDeclaration = *it;

        if (isa<NamedDecl>(otherDeclaration))
        {
            if (cast<NamedDecl>(otherDeclaration)->getNameAsString() == declarationName)
            {
                return true;
            }
        }
    }

    return false;
}

// NOTE: this method returns a source range *including* the opening parenthesis of the call expression
// (possibly followed by some whitespace)
SourceRange PassTransformation::getRealCalleeSourceRange(const CallExpr *Expression)
{
    // get source range for callee
    // unfortunately, clang provides the wrong range for our purpose if explicit template arguments are specified:
    //    foo<bar>()
    //    ^~^ clang only provides this source range

    SourceLocation locationBegin = Expression->getCallee()->getLocStart();

    SourceLocation locationEnd;
    
    if (Expression->getNumArgs() > 0)
    {
        locationEnd = Expression->getArg(0)->getLocStart().getLocWithOffset(-1);
    }
    else
    {
        locationEnd = Expression->getRParenLoc().getLocWithOffset(-1);
    }

    return SourceRange(locationBegin, locationEnd);
}

SourceRange PassTransformation::getDeclaratorSourceRange(FunctionDecl *Declaration)
{
    if (isa<CXXMethodDecl>(Declaration))
    {
        CXXMethodDecl *MethodDeclaration = cast<CXXMethodDecl>(Declaration);

        if (MethodDeclaration->getParent() != MethodDeclaration->getLexicalParent())
        {
            // compute source range of 'qualified' declarator
            //
            // void FOO::bar()
            //      ^------^ this is what we want
            //           ^-^ this is what clang gives us...
            //
            // (did not find a better way...)

            const SourceManager &sourceManager = this->context->getSourceManager();
            const LangOptions &languageOptions = this->context->getLangOpts();

            SourceRange returnTypeSourceRange = MethodDeclaration->getReturnTypeSourceRange();
            SourceLocation realReturnTypeEndLocation = Lexer::getLocForEndOfToken(returnTypeSourceRange.getEnd(), 0, sourceManager, languageOptions);

            return SourceRange(realReturnTypeEndLocation.getLocWithOffset(1), MethodDeclaration->getNameInfo().getLocEnd());
        }
    }

    return Declaration->getNameInfo().getSourceRange();
}

class ASTVisitorTemporaryObject: public RecursiveASTVisitor<ASTVisitorTemporaryObject>
{
private:
    bool foundTemporaryObject;
    std::vector<Expr *> *result;

public:
    bool usesTemporaryObject(Stmt *Statement, std::vector<Expr *> &result)
    {
        this->foundTemporaryObject = false;
        this->result = &result;

        this->TraverseStmt(Statement);

        return this->foundTemporaryObject;
    }

    bool TraverseCompoundStmt(CompoundStmt *Statement)
    {
        // NOTE: we do not want to find temporary objects in nested compound statements
        // -> do not traverse compound statement

        return true;
    }

    bool VisitCXXFunctionalCastExpr(CXXFunctionalCastExpr *Expression)
    {
        this->foundTemporaryObject = true;
        this->result->push_back(Expression);

        return true;
    }

    bool VisitCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *Expression)
    {
        this->foundTemporaryObject = true;
        this->result->push_back(Expression);

        return true;
    }
};

bool PassTransformation::findUsagesOfTemporaryObjects(Stmt *Statement, std::vector<Expr *> &result)
{
    // sooo meta!
    return ASTVisitorTemporaryObject().usesTemporaryObject(Statement, result);
}

std::string PassTransformation::getNameForTemporaryObject(Expr *Expression)
{
    if (this->temporaryObjectNames.find(Expression) != this->temporaryObjectNames.end())
    {
        return this->temporaryObjectNames[Expression];
    }

    std::string temporaryName = "__patos_temporary_" + std::to_string(this->temporaryObjectCounter++);
    this->temporaryObjectNames[Expression] = temporaryName;

    return temporaryName;
}

bool PassTransformation::hasAlreadyATypeDef(const std::string &recordName)
{
    TranslationUnitDecl *translationUnit = this->context->getTranslationUnitDecl();

    for (auto it = translationUnit->decls_begin(); it != translationUnit->decls_end(); ++it)
    {
        Decl *declaration = *it;

        if (isa<TypedefDecl>(declaration) && cast<TypedefDecl>(declaration)->getNameAsString() == recordName)
            return true;
    }

    return false;
}


// =============================================== //
// ===== PASS_TRANSFORMATION: PUBLIC METHODS ===== //
// =============================================== //


void PassTransformation::HandleTranslationUnit(clang::ASTContext &context)
{
    DBG << "Consume [TRANSFORMATION]: " << this->FileName << std::endl;

    if (this->arguments.DumpAST)
    {
        this->dumpDeclarationAST(context.getTranslationUnitDecl(), "transformation");
    }

    this->currentRewriter = this->rewriter;
    this->context = &context;
    this->TraverseDecl(context.getTranslationUnitDecl());

    // write result to disk
    this->writeChangesToDisk();
}

bool PassTransformation::TraverseClassTemplateDecl(ClassTemplateDecl *Declaration)
{
    // don't touch system files
    if (isInSystemFile(Declaration))
    {
        return true;
    }

    DBG << "found class template declaration: " << Declaration->getNameAsString() << std::endl;

    // iterate over all specializations
    for (auto it = Declaration->spec_begin(); it != Declaration->spec_end(); ++it)
    {
        ClassTemplateSpecializationDecl *specializationDeclaration = *it;

        #ifdef DEBUG_MODE
        {
            DBG << "   found specialization: ";

            const TemplateArgumentList &templateArguments = specializationDeclaration->getTemplateArgs();
            for (unsigned idx = 0; idx < templateArguments.size(); ++idx)
            {
                const TemplateArgument &templateArgument = templateArguments.get(idx);
                QualType argumentType = templateArgument.getAsType();

                std::cout << argumentType.getAsString();
                if (idx < templateArguments.size()-1)
                    std::cout << ", ";
            }
            std::cout << std::endl;
        }
        #endif

        // check if we already have an according specialization
        std::string mangledName = PatosNameMangling::getMangledNameForRecord(specializationDeclaration);
        if (!this->hasAlreadyADeclaration(mangledName))
        {
            // save old state
            Rewriter *oldRewriter = this->currentRewriter;

            // create a new rewriter for the current specialization
            Rewriter specializationRewriter;
            specializationRewriter.setSourceMgr(this->context->getSourceManager(), this->context->getLangOpts());
            this->currentRewriter = &specializationRewriter;

            // traverse specialization to perform type substitution, name mangling, call replacement, etc.
            this->currentClassTemplate = Declaration; // TODO do we need this?
            DBG << "begin transformation of specialization of " << Declaration->getNameAsString() << std::endl;
            TraverseCXXRecordDecl(specializationDeclaration);
            DBG << "finished transformation of specialization of " << Declaration->getNameAsString() << std::endl;
            this->currentClassTemplate = NULL;

            // restore old state
            this->currentRewriter = oldRewriter;
        }
    }

    // remove declaration or remember to remove it later (if it is inside an included file)
    removeDeclarationOrRememberToRemoveInTheFuture(Declaration);

    return true;
}

bool PassTransformation::TraverseCXXRecordDecl(CXXRecordDecl *Declaration)
{
    // don't touch system files
    if (isInSystemFile(Declaration))
    {
        return true;
    }

    if (this->currentClassTemplate == NULL)
    {
        DBG << "found record declaration: " << Declaration->getNameAsString() << std::endl;
    }

    // we do not have to do anything, if the record is not a specialization and does not contain any methods
    if (!(isa<ClassTemplateSpecializationDecl>(Declaration) || containsMethods(Declaration)))
    {
        std::string recordName = Declaration->getNameAsString();

        // wrap declaration in typedef (if not yet)
        if (!this->hasAlreadyATypeDef(recordName))
        {
            SourceManager &sourceManager = this->context->getSourceManager();
            const LangOptions &languageOptions = this->context->getLangOpts();

            //SourceLocation locationEnd = Lexer::getLocForEndOfToken(Declaration->getLocEnd(), 0, sourceManager, languageOptions);
            SourceLocation locationEnd = Declaration->getLocEnd();
            locationEnd = Lexer::findLocationAfterToken(locationEnd, tok::semi, sourceManager, languageOptions, true);

            this->currentRewriter->InsertTextAfter(locationEnd, "\ntypedef struct " + recordName + " " + recordName + ";\n");
        }

        return true;
    }

    // get insert location for methods and the version of the record declaration containing only member variables
    SourceManager &sourceManager = this->context->getSourceManager();
    const LangOptions &languageOptions = this->context->getLangOpts();
    SourceLocation insertLocation = Lexer::findLocationAfterToken(Declaration->getLocEnd(), tok::semi, sourceManager, languageOptions, true);

    // create a version of the record containing only the member variables
    {
        std::string flatVersion = this->createFlatVersionOfRecord(Declaration);

        this->rewriter->InsertTextAfter(insertLocation, flatVersion);

        #ifdef DEBUG_MODE
        {
            std::string mangledName;
            if (isa<ClassTemplateSpecializationDecl>(Declaration))
            {
                mangledName = PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(Declaration));
            }
            else
            {
                mangledName = Declaration->getNameAsString();
            }

            DBG << "created flattened version for " << mangledName << std::endl;
        }
        #endif
    }

    // iterate over all methods contained in this record
    for (auto it = Declaration->method_begin(); it != Declaration->method_end(); ++it)
    {
        CXXMethodDecl *methodDeclaration = *it;

        // TODO doesThisDeclarationHaveABody(), hasSkippedBody() ?
        if (!methodDeclaration->isThisDeclarationADefinition())
        {
            // TODO check this
            continue;
        }

        if (isa<CXXConstructorDecl>(methodDeclaration))
        {
            CXXConstructorDecl *constructorDeclaration = cast<CXXConstructorDecl>(methodDeclaration);
            if (constructorDeclaration->isImplicit())
            {
                // we do not need to transform implicit constructors
                continue;
            }
        }

        if (isa<CXXDestructorDecl>(methodDeclaration))
        {
            CXXDestructorDecl *destructorDeclaration = cast<CXXDestructorDecl>(methodDeclaration);
            if (!destructorDeclaration->isImplicit())
            {
                ERROR << "explicit destructors not supported by patos" << std::endl;
                exit(EXIT_FAILURE);
            }
            continue; // do not transform destructor
        }

        // transform method and add to source
        this->transformFunction(methodDeclaration, insertLocation, methodDeclaration->isThisDeclarationADefinition());
    }

    // save old state
    Rewriter *oldRewriter = this->currentRewriter;

    // iterate over all declarations contained in this record to find function template declarations
    for (auto it = Declaration->decls_begin(); it != Declaration->decls_end(); ++it)
    {
        Decl *declaration = *it;

        if (isa<FunctionTemplateDecl>(declaration))
        {
            FunctionTemplateDecl *functionTemplateDeclaration = cast<FunctionTemplateDecl>(declaration);

            // iterate over all specializations
            for (auto itSpec = functionTemplateDeclaration->spec_begin(); itSpec != functionTemplateDeclaration->spec_end(); ++itSpec)
            {
                FunctionDecl *functionTemplateSpecialization = *itSpec;

                if (!isa<CXXMethodDecl>(functionTemplateSpecialization))
                {
                    ERROR << "specialization of template method is not a method" << std::endl;
                    exit(EXIT_FAILURE);
                }

                // create a new rewriter for the current specialization
                Rewriter specializationRewriter;
                specializationRewriter.setSourceMgr(this->context->getSourceManager(), this->context->getLangOpts());
                this->currentRewriter = &specializationRewriter;

                // transform method and add to source
                this->transformFunction(cast<CXXMethodDecl>(functionTemplateSpecialization), insertLocation, functionTemplateSpecialization->isThisDeclarationADefinition());
            }
        }
    }

    // restore old state
    this->currentRewriter = oldRewriter;

    // remove original record declaration
    // NOTE: original declaration of a class template is removed in TraverseClassTemplateDecl()
    if (!isa<ClassTemplateSpecializationDecl>(Declaration))
    {
        // NOTE: we remove the original declaration both if it is in the main file and if it is not,
        // since this is _not_ a template record, and so we may transform it only once
        // (-> not using removeDeclarationOrRememberToRemoveInTheFuture() is intentional!)
        this->rewriter->RemoveText(Declaration->getSourceRange());
    }

    return true;
}

bool PassTransformation::TraverseCXXMethodDecl(CXXMethodDecl *Declaration)
{
    // don't touch system files
    if (isInSystemFile(Declaration))
    {
        return true;
    }

    // check if we have to traverse the method
    // this is _NOT_ the case if this is the 'original' definition belonging to a template record

    CXXRecordDecl *parent = Declaration->getParent();
    bool isFromTemplateRecord = parent->getDescribedClassTemplate() != NULL;

    if (!isa<ClassTemplateSpecializationDecl>(parent) && isFromTemplateRecord)
    {
        // NOTE do not traverse this method declaration
        return true;
    }

    RecursiveASTVisitor::TraverseCXXMethodDecl(Declaration);

    return true;
}

bool PassTransformation::VisitCXXMethodDecl(CXXMethodDecl *Declaration)
{
    // don't touch system files
    if (isInSystemFile(Declaration))
    {
        return true;
    }

    const SourceManager &sourceManager = this->context->getSourceManager();
    const LangOptions &languageOptions = this->context->getLangOpts();

    // get name for parent record
    std::string parentName;
    if (isa<ClassTemplateSpecializationDecl>(Declaration->getParent()))
    {
        parentName = PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(Declaration->getParent()));
    }
    else
    {
        parentName = Declaration->getParent()->getNameAsString();
    }

    // add additional parameter (thisRef)
    if (!isa<CXXConstructorDecl>(Declaration))
    {
        // find location of left parenthesis (took me alsmost an hour to get there...)
        SourceLocation locationEndOfDeclarator = Declaration->getNameInfo().getLocEnd();
        SourceLocation locationLParen = Lexer::findLocationAfterToken(locationEndOfDeclarator, tok::l_paren, sourceManager, languageOptions, true);

        // build string for additional parameter
        std::string additionalParameter = "struct " + parentName + " *thisRef";

        // add additional parameter to list of parameters
        if (Declaration->getNumParams() == 0)
        {
            this->currentRewriter->InsertTextAfter(locationLParen, additionalParameter);
        }
        else
        {
            // if the method has at least one parameter, we *replace* the opening parenthesis '('
            // to add the additional parameter, since the first parameter may be a template type which has
            // to be replaced and clang's rewriter interface does not like touching the same
            // source location more than once
            this->currentRewriter->ReplaceText(
                    SourceRange(locationLParen.getLocWithOffset(-1), locationLParen),
                    "(" + additionalParameter + ",  ");
        }
    }
    else
    {
        // method is a constructor
        // -> add additional local variable(s) and return expression
        Stmt *body = Declaration->getBody();
        if (body != NULL)
        {
            if (!isa<CompoundStmt>(body))
            {
                ERROR << "body of constructor is not a compound statement" << std::endl;
                exit(EXIT_FAILURE);
            }

            CompoundStmt *compoundBody = cast<CompoundStmt>(body);

            SourceLocation locationLBrac = compoundBody->getLBracLoc().getLocWithOffset(1);
            SourceLocation locationRBrac = compoundBody->getRBracLoc();

            // build prologue and epilogue for function
            #define PATOS_CTOR_AUX_VARIABLE "__patos_constructed"
            std::stringstream prologue;
            prologue << "\n\tstruct " << parentName << " "  << PATOS_CTOR_AUX_VARIABLE << ";"
                     << "\n\tstruct " << parentName << " *thisRef = &(" << PATOS_CTOR_AUX_VARIABLE << ");\n";
            std::stringstream epilogue;
            epilogue << "\n\treturn " << PATOS_CTOR_AUX_VARIABLE << ";\n";
            #undef PATOS_CTOR_AUX_VARIABLE

            this->currentRewriter->InsertTextAfter(locationLBrac, prologue.str());
            this->currentRewriter->InsertTextBefore(locationRBrac, epilogue.str());
        }
    }

    return true;
}

bool PassTransformation::TraverseFunctionTemplateDecl(FunctionTemplateDecl *Declaration)
{
    // don't touch system files
    if (isInSystemFile(Declaration))
    {
        return true;
    }

    // iterate over all specializations
    for (auto it = Declaration->spec_begin(); it != Declaration->spec_end(); ++it)
    {
        FunctionDecl *declarationSpecialization = *it;

        // check if we only have an according specialization
        std::string mangledName = PatosNameMangling::getMangledNameForFunction(declarationSpecialization);
        if (!this->hasAlreadyADeclaration(mangledName))
        {
            // 'new' specialization -> transformation needed

            // save old state
            Rewriter *oldRewriter = this->currentRewriter;

            // create a new rewriter for the current specialization
            Rewriter specializationRewriter;
            specializationRewriter.setSourceMgr(this->context->getSourceManager(), this->context->getLangOpts());
            this->currentRewriter = &specializationRewriter;

            // transformation for specialization
            SourceLocation insertLocation = getRealEndLocationForFunctionDeclaration(declarationSpecialization);
            this->transformFunction(declarationSpecialization, insertLocation, Declaration->isThisDeclarationADefinition());

            // restore old state
            this->currentRewriter = oldRewriter;
        }
    }

    // remove function template declaration if it is inside the main file
    // (otherwise, we may not remove it yet, because it may be needed for other files including this file)
    removeDeclarationOrRememberToRemoveInTheFuture(Declaration);
    
    return true;
}

bool PassTransformation::VisitFunctionDecl(FunctionDecl *Declaration)
{
    // don't touch system files
    if (isInSystemFile(Declaration))
    {
        return true;
    }

    DBG << "      found function declaration: " << Declaration->getNameAsString() << std::endl;
    DBG << "         @" << Declaration->getLocStart().printToString(this->context->getSourceManager()) << std::endl;
    DBG << "         templated kind: " << Declaration->getTemplatedKind() << std::endl;

    // check if we have to perform name mangling for this function declaration
    // this is the case if it is a specialization of a template function or if it is a method of a record
    FunctionDecl::TemplatedKind kind = Declaration->getTemplatedKind();
    if (kind == FunctionDecl::TemplatedKind::TK_FunctionTemplateSpecialization ||
        isa<CXXMethodDecl>(Declaration))
    {
        DBG << "         perform name mangling (" << kind << ")" << std::endl;

        // get mangled name for current specialization
        std::string mangledName = PatosNameMangling::getMangledNameForFunction(Declaration);

        DBG << "         mangled name: " << mangledName << std::endl;

        // replace function name
        SourceRange locationDeclarator = this->getDeclaratorSourceRange(Declaration);
        this->currentRewriter->ReplaceText(locationDeclarator, mangledName);
    }

    return true;
}

bool PassTransformation::TraverseTypeLoc(TypeLoc typeLoc)
{
    // get type information from typeLoc
    const Type *type = typeLoc.getTypePtr();

    // check if type is a template specialization
    bool isTemplateSpecialization = false;
    const RecordType *recordType = type->getAsStructureType();
    if (recordType != NULL)
    {
        isTemplateSpecialization = isa<ClassTemplateSpecializationDecl>(recordType->getDecl());
    }

    // if type is a template specialization, we have to replace it with a mangled name
    if (isTemplateSpecialization)
    {
        // get mangled name for specialization of record
        std::string mangledName = PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(recordType->getDecl()));

        // replace with mangled name
        this->currentRewriter->ReplaceText(typeLoc.getSourceRange(), mangledName);

        // NOTE: do _not_ traverse this type
    }
    else
    {
        // check if type is a substituted template type
        // if it is, we have to replace it with the substitution
        if (isa<SubstTemplateTypeParmType>(type))
        {
            // replace with mapped type
            this->currentRewriter->ReplaceText(typeLoc.getSourceRange(), typeLoc.getType().getAsString());
        }

        RecursiveASTVisitor::TraverseTypeLoc(typeLoc);
    }

    return true;
}

bool PassTransformation::TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *Expression)
{
    RecursiveASTVisitor::TraverseCXXOperatorCallExpr(Expression);

    Decl *calleeDeclaration = Expression->getCalleeDecl();

    if (isa<CXXMethodDecl>(calleeDeclaration))
    {
        CXXMethodDecl *methodDeclaration = cast<CXXMethodDecl>(calleeDeclaration);

        // get mangled name
        std::string mangledName = PatosNameMangling::getMangledNameForFunction(methodDeclaration);

        // get arguments
        std::string argumentString;
        {
            std::stringstream strstr;

            // iterate over all arguments
            for (unsigned argIdx = 0; argIdx < Expression->getNumArgs(); ++argIdx)
            {
                const Expr *argument = Expression->getArg(argIdx);

                // check if we have to get the pointer to the argument
                // (first argument, i.e. the instance the operator function is called upon)
                // TODO is this correct?
                if (argIdx == 0)
                {
                    strstr << "&(" << this->expressionToString(argument) << ")";
                }
                else
                {
                    strstr << this->expressionToString(argument);
                }

                if (argIdx < Expression->getNumArgs()-1)
                {
                    strstr << ", ";
                }
            }

            argumentString = strstr.str();
        }

        // replace operator call with a normal function call
        this->currentRewriter->ReplaceText(Expression->getSourceRange(), mangledName + "(" + argumentString + ")");
    }

    return true;
}


bool PassTransformation::VisitCallExpr(CallExpr *Expression)
{
    Decl *calleeDeclaration = Expression->getCalleeDecl();

    if (calleeDeclaration == NULL)
    {
        return true;
    }

    if (!isa<CXXOperatorCallExpr>(Expression))
    {
        if (isa<FunctionDecl>(calleeDeclaration))
        {
            FunctionDecl *calleeFunctionDeclaration = cast<FunctionDecl>(calleeDeclaration);

            if (calleeFunctionDeclaration->getTemplatedKind() == FunctionDecl::TemplatedKind::TK_FunctionTemplateSpecialization)
            {
                // get mangled name
                std::string mangledName = PatosNameMangling::getMangledNameForFunction(calleeFunctionDeclaration);

                // get source range for callee
                // unfortunately, clang provides the wrong range for our purpose if explicit template arguments are specified:
                //    foo<bar>()
                //    ^~^ clang only provides this source range
                SourceRange calleeRange = this->getRealCalleeSourceRange(Expression);

                // NOTE: calleeRange *includes* the opening parenthesis of the callee expression
                // (possibly followed by some whitespace)
                // -> we have to add the oppening parenthesis if we replace the text

                // replace callee with mangled name
                this->currentRewriter->ReplaceText(calleeRange, mangledName + "(");
            }
        }
    }

    return true;
}

bool PassTransformation::TraverseCallExpr(CallExpr *Expression)
{
    bool traverseCalleeRecursively = (Expression->getCalleeDecl() == NULL) ||
                                    !(isa<CXXOperatorCallExpr>(Expression) && isa<CXXMethodDecl>(Expression->getCalleeDecl()));

    // check which parts of the call expression have to be traversed recursively
    if (Expression->getCalleeDecl() == NULL)
    {
        // both callee and paramters have to be traversed recursively
        // -> simply use RecursiveASTVisitor's implementation of TraverseCallExpr
        RecursiveASTVisitor::TraverseCallExpr(Expression);
    }
    else
    {
        // callee must not be traversed recursively, so we have to traverse the call's parameters manually 
        for (auto it = Expression->arg_begin(); it != Expression->arg_end(); ++it)
        {
            Expr *argument = *it;
            // traverse current argument expression recursively
            TraverseStmt(argument);
        }

        // call visit method on call expression (name mangling)
        VisitCallExpr(Expression);
    }

    return true;
}

bool PassTransformation::TraverseCXXMemberCallExpr(CXXMemberCallExpr *Expression)
{
    std::string expressionString = this->expressionToString(Expression);

    DBG << "            member call expression: " << expressionString << std::endl;

    if (!isa<MemberExpr>(Expression->getCallee()))
    {
        // TODO check this
        RecursiveASTVisitor::TraverseCXXMemberCallExpr(Expression);
        return true;
    }

    MemberExpr *Callee = cast<MemberExpr>(Expression->getCallee());

    // 1) additional argument (thisRef)
    {
        std::string calleeRecord;
        if (Callee->getBase()->isImplicitCXXThis() || isa<CXXThisExpr>(Callee->getBase()))
        {
            calleeRecord = "thisRef";
        }
        else
        {
            calleeRecord = expressionToString(Callee->getBase());

            if (!Callee->isArrow())
            {
                calleeRecord = "&" + calleeRecord;
            }
        }

        // get location where additional argument has to be inserted
        if (Expression->getNumArgs() > 0)
        {
            SourceLocation locationAdditionalArgument = Expression->getArg(0)->getLocStart();
            this->currentRewriter->InsertTextBefore(locationAdditionalArgument, calleeRecord + ", ");
        }
        else
        {
            SourceLocation locationAdditionalArgument = Expression->getRParenLoc();
            this->currentRewriter->InsertTextBefore(locationAdditionalArgument, calleeRecord);
        }
    }

    // 2) replace callee
    {
        std::string mangledName = PatosNameMangling::getMangledNameForFunction(cast<FunctionDecl>(Callee->getMemberDecl()));
        DBG << "               -> replace with call to: " << mangledName << std::endl;
        this->currentRewriter->ReplaceText(Callee->getSourceRange(), mangledName);
    }

    // NOTE: we do _not_have to traverse the callee recursively, since we already transformed it
    // however, we have to traverse the argument expressions of the call expression
    for (auto it = Expression->arg_begin(); it != Expression->arg_end(); ++it)
    {
        Expr *argument = *it;
        // traverse current argument expression recursively
        TraverseStmt(argument);
    }

    return true;
}

bool PassTransformation::VisitCXXThisExpr(CXXThisExpr *Expression)
{
    std::string expressionSource = expressionToString(Expression);
    DBG << "            'this' expression: " << expressionSource << std::endl;

    if (Expression->isImplicitCXXThis())
    {
        this->currentRewriter->InsertTextBefore(Expression->getLocStart(), "thisRef->");
    }
    else
    {
        this->currentRewriter->ReplaceText(Expression->getSourceRange(), "thisRef");
    }

    return true;
}

bool PassTransformation::TraverseVarDecl(VarDecl *Declaration)
{
    RecursiveASTVisitor::TraverseVarDecl(Declaration);

    if (Declaration->getInitStyle() == VarDecl::InitializationStyle::CallInit)
    {
        Expr *initExpression = Declaration->getInit();

        if (initExpression == NULL || !isa<CXXConstructExpr>(initExpression))
        {
            ERROR << "unknown initialization of variable '" << Declaration->getNameAsString() << "'" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        CXXConstructExpr *constructExpression = cast<CXXConstructExpr>(initExpression);

        if (!constructExpression->getConstructor()->isImplicit())
        {
            SourceLocation insertLocation;
            {
                SourceRange parenRange = constructExpression->getParenOrBraceRange();
                if (parenRange.isValid())
                {
                    insertLocation = parenRange.getBegin();
                }
                else
                {
                    SourceManager &sourceManager = this->context->getSourceManager();
                    const LangOptions &languageOptions = this->context->getLangOpts();

                    insertLocation = Lexer::getLocForEndOfToken(constructExpression->getLocEnd(), 0, sourceManager, languageOptions);
                }
            }

            this->currentRewriter->InsertTextBefore(insertLocation, " = ");
        }
    }

    return true;
}

bool PassTransformation::TraverseCXXConstructExpr(CXXConstructExpr *Expression)
{
    RecursiveASTVisitor::TraverseCXXConstructExpr(Expression);

    CXXConstructorDecl *constructorDeclaration = Expression->getConstructor();

    if (constructorDeclaration == NULL || constructorDeclaration->isImplicit())
    {
        return true;
    }

    // build call to 'constructor' function
    std::stringstream constructorCall;
    {
        constructorCall << PatosNameMangling::getMangledNameForFunction(constructorDeclaration) << "(";
        unsigned numArgs = Expression->getNumArgs();
        for (unsigned argIdx = 0; argIdx < numArgs; ++argIdx)
        {
            constructorCall << this->expressionToString(Expression->getArg(argIdx));

            if (argIdx < numArgs-1)
            {
                constructorCall << ", ";
            }
        }
        constructorCall << ")";
    }

    SourceRange range = Expression->getParenOrBraceRange();
    if (range.isValid())
    {
        this->currentRewriter->ReplaceText(range, constructorCall.str());
    }
    else
    {
        SourceManager &sourceManager = this->context->getSourceManager();
        const LangOptions &languageOptions = this->context->getLangOpts();

        SourceLocation insertLocation(Lexer::getLocForEndOfToken(Expression->getLocEnd(), 0, sourceManager, languageOptions));

        this->currentRewriter->InsertTextAfter(insertLocation, constructorCall.str());
    }

    return true;
}

bool PassTransformation::TraverseCompoundStmt(CompoundStmt *Block)
{
    for (auto stmtIt = Block->body_begin(); stmtIt != Block->body_end(); ++stmtIt)
    {
        Stmt *statement = *stmtIt;

        if (isa<CompoundStmt>(statement))
        {
            TraverseStmt(statement);
            continue;
        }

        // find usages of temporary objects
        std::vector<Expr *> temporaryObjects;
        if (this->findUsagesOfTemporaryObjects(statement, temporaryObjects))
        {
            // statements uses temporary objects
            // -> create helper variables
            std::stringstream prologue;
            prologue << "/* BEGIN USAGE OF TEMPORARY OBJECT */\n\t";
            {
                // iterate over all temporary objects used by this statement
                // to create a fresh temporary variable for each
                for (auto tempIt = temporaryObjects.begin(); tempIt != temporaryObjects.end(); ++tempIt)
                {
                    Expr *tempExpression = *tempIt;

                    // get type of helper variable
                    std::string typeName;
                    {
                        const Type *type = tempExpression->getType().getTypePtr();

                        if (type == NULL)
                        {
                            ERROR << "temporary expression does not have a known type" << std::endl;
                            exit(EXIT_FAILURE);
                        }

                        CXXRecordDecl *recordDeclaration = type->getAsCXXRecordDecl();

                        if (recordDeclaration == NULL)
                        {
                            ERROR << "type of temporary object is not a record type" << std::endl;
                            exit(EXIT_FAILURE);
                        }

                        if (isa<ClassTemplateSpecializationDecl>(recordDeclaration))
                        {
                            typeName = PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(recordDeclaration));
                        }
                        else
                        {
                            typeName = recordDeclaration->getNameAsString();
                        }
                    }

                    prologue << "struct " << typeName << " " << this->getNameForTemporaryObject(*tempIt);

                    // check if we have to add a call to the 'constructor' function
                    {
                        CXXConstructExpr *constructExpression = NULL;

                        // get constructor expression (if any)
                        if (isa<CXXFunctionalCastExpr>(tempExpression))
                        {
                            Expr *subExpression = cast<CXXFunctionalCastExpr>(tempExpression)->getSubExpr();

                            if (isa<CXXConstructExpr>(subExpression))
                            {
                                constructExpression = cast<CXXConstructExpr>(subExpression);
                            }
                        }
                        else if (isa<CXXTemporaryObjectExpr>(tempExpression))
                        {
                            constructExpression = cast<CXXConstructExpr>(tempExpression);
                        }

                        if (constructExpression == NULL)
                        {
                            ERROR << "did not find a call to a constructor for temporary object" << std::endl;
                            exit(EXIT_FAILURE);
                        }

                        CXXConstructorDecl *constructorDeclaration = constructExpression->getConstructor();

                        // if used constructor is not implicit, we have to add a call to the 'constructor' function
                        if (!constructorDeclaration->isImplicit())
                        {
                            // we do not want the changes we will make to the construct expression to be 'visible' later on
                            // -> work with a new rewriter instance

                            // save old state
                            Rewriter *oldRewriter = this->currentRewriter;

                            // create a new Rewriter
                            Rewriter constructRewriter;
                            constructRewriter.setSourceMgr(this->context->getSourceManager(), this->context->getLangOpts());
                            this->currentRewriter = &constructRewriter;

                            // transform construct expression
                            TraverseCXXConstructExpr(constructExpression);

                            prologue << " = " << constructRewriter.getRewrittenText(constructExpression->getParenOrBraceRange());

                            // restore old state
                            this->currentRewriter = oldRewriter;
                        }
                    }
                    
                    prologue << ";\n\t"; // TODO ctor
                }
            }

            SourceManager &sourceManager = this->context->getSourceManager();
            const LangOptions &languageOptions = this->context->getLangOpts();

            SourceLocation locationBegin = statement->getLocStart();
            SourceLocation locationEnd = statement->getLocEnd();

            SourceLocation locationSemi = Lexer::findLocationAfterToken(locationEnd, tok::semi, sourceManager, languageOptions, true);
            if (locationSemi.isValid())
            {
                locationEnd = Lexer::getLocForEndOfToken(locationSemi, 0, sourceManager, languageOptions);
            }
            else
            {
                locationEnd = Lexer::getLocForEndOfToken(locationEnd, 0, sourceManager, languageOptions);
            }

            TraverseStmt(statement);
            this->currentRewriter->InsertTextBefore(locationBegin, prologue.str());
            this->currentRewriter->InsertTextAfter(locationEnd, "\n\t/* END USAGE OF TEMPORARY OBJECT */\n");
        }
        else
        {
            TraverseStmt(statement);
        }
    }

    return true;
}

bool PassTransformation::TraverseCXXFunctionalCastExpr(CXXFunctionalCastExpr *Expression)
{
    // check we identified this expression as CXXFunctionalCastExpr earlier
    if (this->temporaryObjectNames.find(Expression) == this->temporaryObjectNames.end())
    {
        ERROR << "did not find temporary object earlier (internal error)" << std::endl;
        exit(EXIT_FAILURE);
    }

    // replace expression with the name of the local variable we inserted for this temporary
    std::string temporaryName = this->temporaryObjectNames[Expression];

    this->currentRewriter->ReplaceText(Expression->getSourceRange(), temporaryName);

    // NOTE: do not traverse the expression recursively, since we replaced its source completely

    return true;
}

bool PassTransformation::TraverseCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *Expression)
{
    // check we identified this expression as CXXFunctionalCastExpr earlier
    if (this->temporaryObjectNames.find(Expression) == this->temporaryObjectNames.end())
    {
        ERROR << "did not find temporary object earlier (internal error)" << std::endl;
        exit(EXIT_FAILURE);
    }

    // replace expression with the name of the local variable we inserted for this temporary
    std::string temporaryName = this->temporaryObjectNames[Expression];

    this->currentRewriter->ReplaceText(Expression->getSourceRange(), temporaryName);

    // NOTE: do not traverse the expression recursively, since we replaced its source completely
    
    return true;
}
