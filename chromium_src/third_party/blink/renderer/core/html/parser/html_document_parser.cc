/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/html/parser/html_document_parser.cc"

namespace blink {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
void HTMLDocumentParser::HTMLDocumentParserConstructed() {
  if (CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) {
    // Fully disable preloader if a PageGraph session is active.
    preloader_ = nullptr;
  }
}
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

}  // namespace blink
