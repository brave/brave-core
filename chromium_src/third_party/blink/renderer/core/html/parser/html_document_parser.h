/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PARSER_HTML_DOCUMENT_PARSER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PARSER_HTML_DOCUMENT_PARSER_H_

#include <type_traits>

#include "brave/components/brave_page_graph/common/buildflags.h"

#define AsHTMLParserScriptRunnerHostForTesting                                 \
  NotUsed();                                                                   \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, void HTMLDocumentParserConstructed();) \
  HTMLParserScriptRunnerHost* AsHTMLParserScriptRunnerHostForTesting

#include "src/third_party/blink/renderer/core/html/parser/html_document_parser.h"  // IWYU pragma: export

#undef AsHTMLParserScriptRunnerHostForTesting

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
namespace cppgc {

template <typename T, typename>
struct PostConstructionCallbackTrait;

// This PostConstruction extension adds
// HTMLDocumentParser::HTMLDocumentParserConstructed() call after the
// HTMLDocumentParser construction. We use this to disable HTMLResourcePreloader
// when a PageGraph session is active, which allows us to connect network
// requests with the actual DOM Nodes.

template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<
        std::is_base_of<blink::HTMLDocumentParser, T>::value,
        void>::type> {
  static void Call(blink::HTMLDocumentParser* object) {
    object->HTMLDocumentParserConstructed();
  }
};

}  // namespace cppgc
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PARSER_HTML_DOCUMENT_PARSER_H_
