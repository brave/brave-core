/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/browser/blink/speedreader_background_html_parser.h"

#include <memory>
#include <utility>
#include <string>

#include "base/single_thread_task_runner.h"
#include "base/logging.h"
#include "base/debug/stack_trace.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/html/parser/html_document_parser.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/html/parser/xss_auditor.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

base::WeakPtr<SpeedreaderBackgroundHTMLParser>
  SpeedreaderBackgroundHTMLParser::Create(
    std::unique_ptr<Configuration> config,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner) {
  auto* background_parser = new SpeedreaderBackgroundHTMLParser(
      std::move(config), std::move(loading_task_runner));
  return background_parser->weak_factory_.GetWeakPtr();
}

void SpeedreaderBackgroundHTMLParser::Init(
    const KURL& document_url,
    std::unique_ptr<CachedDocumentParameters> cached_document_parameters,
    const MediaValuesCached::MediaValuesCachedData& media_values_cached_data,
    bool priority_hints_origin_trial_enabled) {
  speedreader_.reset(document_url.GetString().Utf8().data());

  preload_scanner_.reset(new TokenPreloadScanner(
      document_url, std::move(cached_document_parameters),
      media_values_cached_data, TokenPreloadScanner::ScannerType::kMainDocument,
      priority_hints_origin_trial_enabled));
}

SpeedreaderBackgroundHTMLParser::SpeedreaderBackgroundHTMLParser(
    std::unique_ptr<Configuration> config,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner)
    : BackgroundHTMLParser(std::move(config), std::move(loading_task_runner)),
      speedreader_(),
      weak_factory_(this) {}

SpeedreaderBackgroundHTMLParser::~SpeedreaderBackgroundHTMLParser() = default;

void SpeedreaderBackgroundHTMLParser::AppendDecodedBytes(const String& input) {
  DCHECK(!input_.Current().IsClosed());
  input_.Append(input);
  speedreader_.pumpContent(input.Utf8().data());
}

void SpeedreaderBackgroundHTMLParser::Finish() {
  std::string transformed;
  bool readable = speedreader_.finalize(&transformed);
  if (readable) {
    if (parser_) {
      parser_->HTMLInterventionActive();
    }
    DCHECK(!input_.Current().IsClosed());
    input_.Current().Clear();
    input_.Append(decoder_->Decode(transformed.c_str(), transformed.length()));
  }

  MarkEndOfFile();
  PumpTokenizer();
}


}  // namespace blink
