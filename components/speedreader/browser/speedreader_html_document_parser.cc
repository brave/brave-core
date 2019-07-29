/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/browser/speedreader_html_document_parser.h"

#include <memory>
#include <utility>

#include "base/auto_reset.h"
#include "base/numerics/safe_conversions.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_loading_behavior_flag.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/css/media_values_cached.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/dom/document_fragment.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/html_document.h"
#include "third_party/blink/renderer/core/html/parser/atomic_html_token.h"
#include "third_party/blink/renderer/core/html/parser/background_html_parser.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_scheduler.h"
#include "third_party/blink/renderer/core/html/parser/html_resource_preloader.h"
#include "third_party/blink/renderer/core/html/parser/html_tree_builder.h"
#include "third_party/blink/renderer/core/html/parser/pump_session.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/navigation_scheduler.h"
#include "third_party/blink/renderer/core/loader/preload_helper.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/script/html_parser_script_runner.h"
#include "third_party/blink/renderer/platform/bindings/runtime_call_stats.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"

namespace blink {

using namespace html_names;

// This is a direct transcription of step 4 from:
// http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#fragment-case
static HTMLTokenizer::State TokenizerStateForContextElement(
    Element* context_element,
    bool report_errors,
    const HTMLParserOptions& options) {
  if (!context_element)
    return HTMLTokenizer::kDataState;

  const QualifiedName& context_tag = context_element->TagQName();

  if (context_tag.Matches(kTitleTag) || context_tag.Matches(kTextareaTag))
    return HTMLTokenizer::kRCDATAState;
  if (context_tag.Matches(kStyleTag) || context_tag.Matches(kXmpTag) ||
      context_tag.Matches(kIFrameTag) || context_tag.Matches(kNoembedTag) ||
      (context_tag.Matches(kNoscriptTag) && options.script_enabled) ||
      context_tag.Matches(kNoframesTag))
    return report_errors ? HTMLTokenizer::kRAWTEXTState
                         : HTMLTokenizer::kPLAINTEXTState;
  if (context_tag.Matches(kScriptTag))
    return report_errors ? HTMLTokenizer::kScriptDataState
                         : HTMLTokenizer::kPLAINTEXTState;
  if (context_tag.Matches(kPlaintextTag))
    return HTMLTokenizer::kPLAINTEXTState;
  return HTMLTokenizer::kDataState;
}

SpeedreaderHTMLDocumentParser::SpeedreaderHTMLDocumentParser(
  HTMLDocument& document, ParserSynchronizationPolicy sync_policy)
    : SpeedreaderHTMLDocumentParser(document,
                        kAllowScriptingContent,
                        sync_policy) {
  script_runner_ =
      HTMLParserScriptRunner::Create(ReentryPermit(), &document, this);
  tree_builder_ =
      HTMLTreeBuilder::Create(this, document, kAllowScriptingContent, options_);
  }

SpeedreaderHTMLDocumentParser::SpeedreaderHTMLDocumentParser(
    DocumentFragment* fragment,
    Element* context_element,
    ParserContentPolicy parser_content_policy)
    : SpeedreaderHTMLDocumentParser(fragment->GetDocument(),
                         parser_content_policy,
                         kForceSynchronousParsing) {
  // No script_runner_ in fragment parser.
  tree_builder_ = HTMLTreeBuilder::Create(this, fragment, context_element,
                                          parser_content_policy, options_);

  // For now document fragment parsing never reports errors.
  bool report_errors = false;
  tokenizer_->SetState(TokenizerStateForContextElement(
      context_element, report_errors, options_));
  xss_auditor_.InitForFragment();
    }


SpeedreaderHTMLDocumentParser::SpeedreaderHTMLDocumentParser(Document& document,
                                       ParserContentPolicy content_policy,
                                       ParserSynchronizationPolicy sync_policy)
    : HTMLDocumentParser(document, content_policy, sync_policy) { }

SpeedreaderHTMLDocumentParser::~SpeedreaderHTMLDocumentParser() = default;


void SpeedreaderHTMLDocumentParser::StartBackgroundParser() {
  DCHECK(!IsStopped());
  DCHECK(ShouldUseThreading());
  DCHECK(!have_background_parser_);
  DCHECK(GetDocument());
  have_background_parser_ = true;

  // Make sure that a resolver is set up, so that the correct viewport
  // dimensions will be fed to the background parser and preload scanner.
  if (GetDocument()->Loader())
    GetDocument()->EnsureStyleResolver();

  std::unique_ptr<BackgroundHTMLParser::Configuration> config =
      std::make_unique<BackgroundHTMLParser::Configuration>();
  config->options = options_;
  config->parser = weak_factory_.GetWeakPtr();
  config->xss_auditor = std::make_unique<XSSAuditor>();
  config->xss_auditor->Init(GetDocument(), &xss_auditor_delegate_);

  config->decoder = TakeDecoder();

  DCHECK(config->xss_auditor->IsSafeToSendToAnotherThread());

  bool speedreader_enabled = false;
  if (auto* document = GetDocument()) {
    if (auto* frame = document->GetFrame()) {
      if (frame->IsMainFrame()) {
        if (auto* settings_client = frame->GetContentSettingsClient()) {
          if (settings_client->RunSpeedreader(document->GetFrame())) {
            speedreader_enabled = true;
          } else {
          }
        }
      }
    }
  }

  // The background parser is created on the main thread, but may otherwise
  // only be used from the parser thread.
  if (speedreader_enabled) {
    background_parser_ =
      SpeedreaderBackgroundHTMLParser::Create(std::move(config),
        loading_task_runner_);
  } else {
    background_parser_ =
      BackgroundHTMLParser::Create(std::move(config), loading_task_runner_);
  }

  // TODO(csharrison): This is a hack to initialize MediaValuesCached on the
  // correct thread. We should get rid of it.

  // TODO(domfarolino): Remove this once Priority Hints is no longer in Origin
  // Trial. This currently exists because the TokenPreloadScanner needs to know
  // the status of the Priority Hints Origin Trial, and has no way of figuring
  // this out on its own. See https://crbug.com/821464.
  bool priority_hints_origin_trial_enabled =
      RuntimeEnabledFeatures::PriorityHintsEnabled(GetDocument());

  background_parser_->Init(
      GetDocument()->Url(),
      std::make_unique<CachedDocumentParameters>(GetDocument()),
      MediaValuesCached::MediaValuesCachedData(*GetDocument()),
      priority_hints_origin_trial_enabled);
}

void SpeedreaderHTMLDocumentParser::HTMLInterventionActive() {
  if (auto* document = GetDocument()) {
    if (auto* frame = document->GetFrame()) {
      if (auto* settings_client = frame->GetContentSettingsClient()) {
        settings_client->DidTransformSpeedreader();
      }
    }
  }
}

} // namespace blink
