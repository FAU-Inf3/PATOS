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

#ifndef __INCLUDE_PARSE_H
#define __INCLUDE_PARSE_H

#include <string>
#include <vector>
#include <utility>

#include "patos_consumer.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/SourceManager.h"

typedef std::pair<std::string, clang::SrcMgr::CharacteristicKind> IncludePath;

/**
 * TODO comment
 */
void parseAndConsume(const std::string &fileName, PatosConsumer &consumer, std::vector<IncludePath> &includePaths, bool CPlusPlus = true, bool OpenCL = false);

#endif
