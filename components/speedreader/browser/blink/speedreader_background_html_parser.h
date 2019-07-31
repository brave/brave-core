/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_BROWSER_BLINK_SPEEDREADER_BACKGROUND_HTML_PARSER_H_ // NOLINT
#define BRAVE_COMPONENTS_SPEEDREADER_BROWSER_BLINK_SPEEDREADER_BACKGROUND_HTML_PARSER_H_ // NOLINT

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "third_party/blink/renderer/core/html/parser/background_html_parser.h"
#include "brave/vendor/speedreader_rust_ffi/src/wrapper.hpp"

namespace blink {

class SpeedreaderHTMLDocumentParser;
class XSSAuditor;

class SpeedreaderBackgroundHTMLParser: public BackgroundHTMLParser {
  USING_FAST_MALLOC(SpeedreaderBackgroundHTMLParser);

 public:
  // The returned SpeedreaderBackgroundHTMLParser must first be initialized
  // by calling init(), and free by calling stop().
  static base::WeakPtr<SpeedreaderBackgroundHTMLParser> Create(
      std::unique_ptr<Configuration>,
      scoped_refptr<base::SingleThreadTaskRunner>);
  void Init(const KURL& document_url,
            std::unique_ptr<CachedDocumentParameters>,
            const MediaValuesCached::MediaValuesCachedData&,
            bool priority_hints_origin_trial_enabled) override;

  void Finish() override;

 private:
  friend class BackgroundHTMLParser;
  using BackgroundHTMLParser::BackgroundHTMLParser;
  SpeedreaderBackgroundHTMLParser(std::unique_ptr<Configuration>,
                       scoped_refptr<base::SingleThreadTaskRunner>);
  ~SpeedreaderBackgroundHTMLParser() override;

  void AppendDecodedBytes(const String&) override;

  // base::WeakPtr<SpeedreaderHTMLDocumentParser> parser_;
  speedreader::SpeedReader speedreader_;

  base::WeakPtrFactory<SpeedreaderBackgroundHTMLParser> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpeedreaderBackgroundHTMLParser);
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_SPEEDREADER_BROWSER_BLINK_SPEEDREADER_BACKGROUND_HTML_PARSER_H_
