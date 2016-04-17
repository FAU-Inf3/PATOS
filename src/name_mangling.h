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

#ifndef __INCLUDE_NAME_MANGLINNAME_MANGLING
#define __INCLUDE_NAME_MANGLINNAME_MANGLING

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <iterator>

#include "clang/AST/Type.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/OperatorKinds.h"

using namespace clang;


// ========== NAME MANGLING ========== 

namespace PatosNameMangling
{
    extern const std::string MANGLED_NAME_FUNCTION_PREFIX;
    extern const std::string MANGLED_NAME_RECORD_PREFIX;
    extern const std::string MANGLED_NAME_TYPE_DELIMITER;
    extern const std::string MANGLED_NAME_METHOD_RECORD_SEPARATOR;
    extern const std::string MANGLED_NAME_OPERATOR;

    std::string getMangledNameForFunction(const FunctionDecl *Declaration);
    std::string getMangledNameForRecord(const ClassTemplateSpecializationDecl *Declaration);

    std::string getMangledNameForKernel(const std::string &kernelName, const std::vector<std::string> &templateArguments);
};

#endif
