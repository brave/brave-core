/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_BROWSER_BLINK_SPEEDREADER_HTML_DOCUMENT_PARSER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_BROWSER_BLINK_SPEEDREADER_HTML_DOCUMENT_PARSER_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/parser/html_document_parser.h"
#include "brave/components/speedreader/browser/blink/speedreader_background_html_parser.h"

namespace blink {

class SpeedreaderBackgroundHTMLParser;

class CORE_EXPORT SpeedreaderHTMLDocumentParser : public HTMLDocumentParser {
  USING_GARBAGE_COLLECTED_MIXIN(SpeedreaderHTMLDocumentParser);
  USING_PRE_FINALIZER(SpeedreaderHTMLDocumentParser, Dispose);

 public:
  static SpeedreaderHTMLDocumentParser* Create(
      HTMLDocument& document,  // NOLINT
      ParserSynchronizationPolicy background_parsing_policy) {
    return MakeGarbageCollected<SpeedreaderHTMLDocumentParser>(document,
                                                    background_parsing_policy);
  }

  SpeedreaderHTMLDocumentParser(HTMLDocument&, ParserSynchronizationPolicy);
  SpeedreaderHTMLDocumentParser(DocumentFragment*,
                     Element* context_element,
                     ParserContentPolicy);
  ~SpeedreaderHTMLDocumentParser() override;

  void HTMLInterventionActive() override;

 private:
  friend class HTMLDocumentParser;
  static SpeedreaderHTMLDocumentParser* Create(DocumentFragment* fragment,
                                    Element* context_element,
                                    ParserContentPolicy parser_content_policy) {
    return MakeGarbageCollected<SpeedreaderHTMLDocumentParser>(
      fragment, context_element, parser_content_policy);
  }
  SpeedreaderHTMLDocumentParser(Document&,
                     ParserContentPolicy,
                     ParserSynchronizationPolicy);

  void StartBackgroundParser() override;
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_SPEEDREADER_BROWSER_BLINK_SPEEDREADER_HTML_DOCUMENT_PARSER_H_
