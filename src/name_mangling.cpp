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

#include <sstream>
#include <map>
#include <vector>

#include "name_mangling.h"
#include "common.h"

const std::string PatosNameMangling::MANGLED_NAME_FUNCTION_PREFIX = "__patos_";
const std::string PatosNameMangling::MANGLED_NAME_RECORD_PREFIX = "__Patos_";
const std::string PatosNameMangling::MANGLED_NAME_TYPE_DELIMITER = "_";
const std::string PatosNameMangling::MANGLED_NAME_METHOD_RECORD_SEPARATOR = "__";
const std::string PatosNameMangling::MANGLED_NAME_OPERATOR = "operator__";


// ========== NAME MANGLING ========== 

std::string PatosNameMangling::getMangledNameForFunction(const FunctionDecl *Declaration)
{
    std::stringstream strstr;

    if (isa<CXXMethodDecl>(Declaration))
    {
        const CXXRecordDecl *parent = cast<CXXMethodDecl>(Declaration)->getParent();

        strstr << PatosNameMangling::MANGLED_NAME_RECORD_PREFIX << parent->getNameAsString();

        if (isa<ClassTemplateSpecializationDecl>(parent))
        {
            const ClassTemplateSpecializationDecl *classTemplateSpecialization = cast<ClassTemplateSpecializationDecl>(parent);
            const TemplateArgumentList &templateArguments = classTemplateSpecialization->getTemplateArgs();

            for (int i = 0; i < templateArguments.size(); ++i)
            {
                strstr << PatosNameMangling::MANGLED_NAME_TYPE_DELIMITER << templateArguments.get(i).getAsType().getAsString();
            }
        }

        strstr << PatosNameMangling::MANGLED_NAME_METHOD_RECORD_SEPARATOR;
    }
    else
    {
        strstr << PatosNameMangling::MANGLED_NAME_FUNCTION_PREFIX;
    }

    if (Declaration->isOverloadedOperator())
    {
        const OverloadedOperatorKind operatorKind = Declaration->getOverloadedOperator();

        if (operatorKind < 0 || operatorKind >= OverloadedOperatorKind::NUM_OVERLOADED_OPERATORS)
        {
            ERROR << "invalid operator kind (" << operatorKind << ")" << std::endl;
            exit(EXIT_FAILURE);
        }

        static const std::string operatorNames[OverloadedOperatorKind::NUM_OVERLOADED_OPERATORS] =
            {
                "none", "new", "delete", "array_new", "array_delete", "plus", "minus", "star", "slash", "percent",
                "caret", "amp", "pipe", "tilde", "exclaim", "equal", "less", "greater", "plus_equal",
                "minus_equal", "star_equal", "slash_equal", "percent_equal", "caret_equal", "amp_equal",
                "pipe_equal", "less_less", "greater_greater", "less_less_equal", "greater_greater_equal",
                "equal_equal", "exclaim_equal", "less_equal", "greater_equal", "amp_amp", "pipe_pipe",
                "plus_plus", "minus_minus", "comma", "arrow_star", "arrow", "call", "subscript", "conditional"
            };

        strstr << PatosNameMangling::MANGLED_NAME_OPERATOR
               << operatorNames[operatorKind];
    }
    else
    {
        if (isa<CXXConstructorDecl>(Declaration))
        {
            strstr << "constructor";
        }
        else
        {
            strstr << Declaration->getNameAsString();
        }
    }

    // if method has template arguments, we have to add them to the mangled name
    const TemplateArgumentList *templateArguments = Declaration->getTemplateSpecializationArgs();
    if (templateArguments != NULL)
    {
        for (int i = 0; i < templateArguments->size(); ++i)
        {
            strstr << PatosNameMangling::MANGLED_NAME_TYPE_DELIMITER;

            QualType templateQualType = templateArguments->get(i).getAsType();
            const Type *templateType = templateQualType.getTypePtr();

            CXXRecordDecl *recordDeclaration = templateType->getAsCXXRecordDecl();

            if (recordDeclaration != NULL && isa<ClassTemplateSpecializationDecl>(recordDeclaration))
            {
                strstr << PatosNameMangling::getMangledNameForRecord(cast<ClassTemplateSpecializationDecl>(recordDeclaration));
            }
            else
            {
                strstr << templateQualType.getAsString();
            }
        }
    }

    return strstr.str();
}

std::string PatosNameMangling::getMangledNameForRecord(const ClassTemplateSpecializationDecl *Declaration)
{
    std::stringstream strstr;

    strstr << PatosNameMangling::MANGLED_NAME_RECORD_PREFIX << Declaration->getNameAsString();

    const TemplateArgumentList &templateArguments = Declaration->getTemplateArgs();
    for (int i = 0; i < templateArguments.size(); ++i)
    {
        strstr << PatosNameMangling::MANGLED_NAME_TYPE_DELIMITER << templateArguments.get(i).getAsType().getAsString();
    }

    return strstr.str(); // strstrstrstrstrstr...
}

std::string PatosNameMangling::getMangledNameForKernel(const std::string &kernelName, const std::vector<std::string> &templateArguments)
{
    std::stringstream strstr;

    strstr << PatosNameMangling::MANGLED_NAME_FUNCTION_PREFIX << kernelName;

    for (auto it = templateArguments.begin(); it != templateArguments.end(); ++it)
    {
        std::string templateArgument = *it;
        strstr << PatosNameMangling::MANGLED_NAME_TYPE_DELIMITER << templateArgument;
    }

    return strstr.str();
}
