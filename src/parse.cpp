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
#include <sstream>

#include "parse.h"
#include "common.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/DirectoryLookup.h"
#include "clang/Lex/HeaderSearch.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

#define NUM_OPENCL_KEYWORDS 8
static const std::string OPENCL_KEYWORDS[NUM_OPENCL_KEYWORDS] =
                    {
                        "__global",
                        "__local",
                        "__constant",
                        "__private",
                        "__read_only",
                        "__write_only",
                        "__read_write",
                        "__kernel"
                    };

void parseAndConsume(const std::string &fileName, PatosConsumer &consumer, std::vector<IncludePath> &includePaths, bool CPlusPlus, bool OpenCL)
{
    // create a compiler instance that will hold the clang compiler
    // along with all the necessary data structures to run the compiler
    CompilerInstance compiler;

    // setup compiler instance
    {
        DBG << "setup compiler instance" << std::endl;

        // create the diagnostics engine for the compiler
        compiler.createDiagnostics();

        // LangOptions defines the language to be parsed by clang
        LangOptions &languageOptions = compiler.getLangOpts();

        if (CPlusPlus)
        {
            languageOptions.CPlusPlus = 1;
        }

        if (OpenCL)
        {
            languageOptions.OpenCL = 1;
        }

        // set the target info with default values
        std::shared_ptr<TargetOptions> targetOptions = std::make_shared<TargetOptions>();
        targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
        TargetInfo *targetInfo = clang::TargetInfo::CreateTargetInfo(compiler.getDiagnostics(), targetOptions);
        compiler.setTarget(targetInfo);

        // create a file manager and a source manager
        compiler.createFileManager();
        FileManager &fileManager = compiler.getFileManager();
        compiler.createSourceManager(fileManager);
        SourceManager &sourceManager = compiler.getSourceManager();

        // create preprocessor and context
        compiler.createPreprocessor(TU_Module);
        compiler.createASTContext();

        // add header search paths
        HeaderSearch &headerSearch = compiler.getPreprocessor().getHeaderSearchInfo();
        for (auto it = includePaths.begin(); it != includePaths.end(); ++it)
        {
            IncludePath includePath = *it;

            const DirectoryEntry *directoryEntry = fileManager.getDirectory(includePath.first);

            if (directoryEntry == NULL)
            {
                // directory does not exist
                continue;
            }

            DirectoryLookup directoryLookup(directoryEntry, includePath.second, false);

            headerSearch.AddSearchPath(directoryLookup, includePath.second == clang::SrcMgr::CharacteristicKind::C_System);
        }

        // define built-in types as macros (don't know why libclang does not provide this...)
        std::stringstream predefines;
        predefines << "#define __SIZE_TYPE__ unsigned" << std::endl;
        predefines << "#define __WINT_TYPE__ unsigned" << std::endl;

        // define OpenCL keywords as macros
        if (!OpenCL)
        {
            for (unsigned idx = 0; idx < NUM_OPENCL_KEYWORDS; ++idx)
            {
                predefines << "#define " << OPENCL_KEYWORDS[idx] << " __attribute__ ((annotate(\"__patos" << OPENCL_KEYWORDS[idx] << "\")))" << std::endl;
            }
        }

        // set predefines
        Preprocessor &preprocessor = compiler.getPreprocessor();
        preprocessor.setPredefines(predefines.str());

        // add the current file to the compiler instance's file manager
        const FileEntry *inputFile = fileManager.getFile(fileName);
        sourceManager.setMainFileID(sourceManager.createFileID(inputFile, SourceLocation(), SrcMgr::C_User));
        compiler.getDiagnosticClient().BeginSourceFile(compiler.getLangOpts(), &compiler.getPreprocessor());
    }

    // create a rewriter
    Rewriter rewriter;
    rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());

    // setup consumer and parse file
    DBG << "start parsing " << fileName << std::endl;

    consumer.setRewriter(&rewriter);
    consumer.setSourceManager(&compiler.getSourceManager());
    ParseAST(compiler.getPreprocessor(), &consumer, compiler.getASTContext());

    DBG << "finished parsing " << fileName << std::endl;
}
