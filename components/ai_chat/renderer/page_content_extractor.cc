// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/page_content_extractor.h"

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_tree.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/core/common/utils.h"
#include "brave/components/ai_chat/core/common/yt_util.h"
#include "brave/components/ai_chat/renderer/page_text_distilling.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-local-handle.h"

namespace ai_chat {

namespace {

constexpr char16_t kYoutubeInnerTubeConfigExtractionScript[] =
    uR"JS(
      (function() {
        // Get API key from ytcfg or fallback to regex
        const apiKey = window.ytcfg?.data_?.INNERTUBE_API_KEY || (() => {
          const scripts = document.querySelectorAll('script');
          for (const script of scripts) {
            const match = script.textContent?.match(
                /"INNERTUBE_API_KEY":"([^"]+)"/);
            if (match) return match[1];
          }
          return null;
        })();
        // Get video ID from URL
        const videoId = new URLSearchParams(window.location.search).get('v');
        if (!videoId || !apiKey) return null;

        // Return the API request configuration
        return {
          type: 'youtube_inner_tube',
          apiKey: apiKey,
          videoId: videoId
        };
      })()
    )JS";

constexpr char16_t kVideoTrackTranscriptUrlExtractionScript[] =
    // TODO(petemill): Make more informed srclang choice.
    // TODO(petemill): Observe <video>.textTracks
    uR"JS(
      (function() {
        const nodes = document.querySelectorAll('video track')
        if (nodes.length) {
          let selectedNode = nodes[0]
          for (const node of nodes) {
            if (node.srclang.toLowerCase() === 'en') {
              selectedNode = node
            }
          }
          return node.src
        }
      })()
    )JS";

// TODO(petemill): Use heuristics to determine if page's main focus is
// a video, and not a hard-coded list of Url hosts.
constexpr auto kVideoTrackHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "www.ted.com",
                                             });

}  // namespace

PageContentExtractor::PageContentExtractor(
    content::RenderFrame* render_frame,
    service_manager::BinderRegistry* registry,
    int32_t global_world_id,
    int32_t isolated_world_id)
    : content::RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PageContentExtractor>(render_frame),
      global_world_id_(global_world_id),
      isolated_world_id_(isolated_world_id),
      weak_ptr_factory_(this) {
  if (!render_frame->IsMainFrame()) {
    return;
  }
  // Bind mojom API to allow browser to communicate with this class
  // Being a RenderFrameObserver, this object is scoped to the RenderFrame.
  // Unretained is safe here because `registry` is also scoped to the
  // RenderFrame.
  registry->AddInterface(base::BindRepeating(
      &PageContentExtractor::BindReceiver, base::Unretained(this)));
}

PageContentExtractor::~PageContentExtractor() = default;

void PageContentExtractor::OnDestruct() {
  delete this;
}

base::WeakPtr<PageContentExtractor> PageContentExtractor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PageContentExtractor::ExtractPageContent(
    mojom::PageContentExtractor::ExtractPageContentCallback callback) {
  VLOG(1) << "AI Chat renderer has been asked for page content.";

  blink::WebLocalFrame* main_frame = render_frame()->GetWebFrame();
  GURL origin =
      url::Origin(((const blink::WebFrame*)main_frame)->GetSecurityOrigin())
          .GetURL();

  // Decide which technique to use to extract content from page...
  // 1) Video - YouTube's custom link to transcript
  // 2) Video - <track> element specifying text location
  // 3) Text - find the "main" text of the page

  if (origin.is_valid()) {
    std::string_view host = origin.host();
    if (kYouTubeHosts.contains(host)) {
      VLOG(1) << "YouTube transcript type";
      // Do Youtube extraction
      v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
      blink::WebScriptSource source = blink::WebScriptSource(
          blink::WebString::FromUTF16(kYoutubeInnerTubeConfigExtractionScript));
      auto script_callback = base::BindOnce(
          &PageContentExtractor::OnJSYoutubeInnerTubeConfigResult,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback),
          ai_chat::mojom::PageContentType::VideoTranscriptYouTube);
      // Main world so that we can access a global variable
      main_frame->RequestExecuteScript(
          global_world_id_, base::span_from_ref(source),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::EvaluationTiming::kAsynchronous,
          blink::mojom::LoadEventBlockingOption::kDoNotBlock,
          std::move(script_callback), blink::BackForwardCacheAware::kAllow,
          blink::mojom::WantResultOption::kWantResult,
          blink::mojom::PromiseResultOption::kAwait);
      return;
    } else if (kVideoTrackHosts.contains(host)) {
      VLOG(1) << "Video track transcript type";
      // Video <track> extraction
      // TODO(petemill): Use heuristics to determine if page's main focus is
      // a video, and not a hard-coded list of Url hosts.
      v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
      blink::WebScriptSource source =
          blink::WebScriptSource(blink::WebString::FromUTF16(
              kVideoTrackTranscriptUrlExtractionScript));
      auto script_callback = base::BindOnce(
          &PageContentExtractor::OnJSTranscriptUrlResult,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback),
          ai_chat::mojom::PageContentType::VideoTranscriptYouTube);
      // Main world so that we can access a global variable
      main_frame->RequestExecuteScript(
          isolated_world_id_, base::span_from_ref(source),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::EvaluationTiming::kAsynchronous,
          blink::mojom::LoadEventBlockingOption::kDoNotBlock,
          std::move(script_callback), blink::BackForwardCacheAware::kAllow,
          blink::mojom::WantResultOption::kWantResult,
          blink::mojom::PromiseResultOption::kAwait);
      return;
    }
  }
  VLOG(1) << "Text transcript type";
  // Do text extraction
  DistillPageText(
      render_frame(), global_world_id_, isolated_world_id_,
      base::BindOnce(&PageContentExtractor::OnDistillResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PageContentExtractor::BindReceiver(
    mojo::PendingReceiver<mojom::PageContentExtractor> receiver) {
  VLOG(1) << "AIChat PageContentExtractor handler bound.";
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void PageContentExtractor::OnDistillResult(
    mojom::PageContentExtractor::ExtractPageContentCallback callback,
    const std::optional<std::string>& content) {
  // Validate
  if (!content.has_value()) {
    VLOG(1) << "null content";
    std::move(callback).Run({});
    return;
  }
  if (content->empty()) {
    VLOG(1) << "Empty string";
    std::move(callback).Run({});
    return;
  }
  VLOG(1) << "Got a distill result of character length: " << content->length();
  // Successful text extraction
  auto result = mojom::PageContent::New();
  result->type = std::move(mojom::PageContentType::Text);
  result->content = mojom::PageContentData::NewContent(content.value());
  std::move(callback).Run(std::move(result));
}

void PageContentExtractor::OnJSYoutubeInnerTubeConfigResult(
    ai_chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
    ai_chat::mojom::PageContentType type,
    std::optional<base::Value> value,
    base::TimeTicks start_time) {
  DVLOG(2) << "InnerTube config extraction script completed and took"
           << (base::TimeTicks::Now() - start_time).InMillisecondsF() << "ms"
           << "\nResult: " << (value ? value->DebugString() : "[undefined]");

  // Handle no result from script
  if (!value.has_value() || !value->is_dict()) {
    std::move(callback).Run({});
    return;
  }

  // Handle InnerTube API request
  const auto& dict = value->GetDict();
  const std::string* api_key = dict.FindString("apiKey");
  const std::string* video_id = dict.FindString("videoId");

  if (!api_key || !video_id) {
    std::move(callback).Run({});
    return;
  }

  // Validate API key - should not be empty and should contain only printable
  // ASCII
  if (api_key->empty()) {
    DVLOG(1) << "Empty API key";
    std::move(callback).Run({});
    return;
  }

  // Check that API key contains only printable ASCII characters
  for (char c : *api_key) {
    if (!base::IsAsciiPrintable(c)) {
      DVLOG(1) << "Invalid character in API key: " << c;
      std::move(callback).Run({});
      return;
    }
  }

  // Validate YouTube video ID - should contain only valid characters
  // YouTube video IDs are defined as [\\w-]+ (word characters + hyphens)
  // Based on Google Closure Library:
  // third_party/google-closure-library/closure/goog/ui/media/youtube.js
  if (video_id->empty()) {
    DVLOG(1) << "Empty video ID";
    std::move(callback).Run({});
    return;
  }
  for (char c : *video_id) {
    if (!base::IsAsciiPrintable(c) ||
        (!base::IsAsciiAlphaNumeric(c) && c != '_' && c != '-')) {
      DVLOG(1) << "Invalid character in video ID: " << c;
      std::move(callback).Run({});
      return;
    }
  }

  // Return the configuration for the browser process to make the API request
  auto result = ai_chat::mojom::PageContent::New();
  result->type = std::move(type);
  result->content = mojom::PageContentData::NewYoutubeInnerTubeConfig(
      mojom::YoutubeInnerTubeConfig::New(*api_key, *video_id));
  std::move(callback).Run(std::move(result));
}

void PageContentExtractor::OnJSTranscriptUrlResult(
    ai_chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
    ai_chat::mojom::PageContentType type,
    std::optional<base::Value> value,
    base::TimeTicks start_time) {
  DVLOG(2) << "Video transcript Url extraction script completed and took"
           << (base::TimeTicks::Now() - start_time).InMillisecondsF() << "ms"
           << "\nResult: " << (value ? value->DebugString() : "[undefined]");

  // Handle no result from script
  if (!value.has_value() || !value->is_list()) {
    std::move(callback).Run({});
    return;
  }

  // Optional parsing
  std::string url;
  if (type == mojom::PageContentType::VideoTranscriptYouTube) {
    auto maybe_url = ChooseCaptionTrackUrl(value->GetList());
    if (maybe_url.has_value()) {
      url = maybe_url.value();
    }
  } else if (value->is_string()) {
    url = value->GetString();
  }
  if (url.empty()) {
    std::move(callback).Run({});
    return;
  }
  // Handle invalid url
  GURL transcript_url =
      render_frame()->GetWebFrame()->GetDocument().CompleteURL(
          blink::WebString::FromASCII(url));
  if (!transcript_url.is_valid() ||
      !transcript_url.SchemeIs(url::kHttpsScheme)) {
    DVLOG(1) << "Invalid Url for transcript: " << transcript_url.spec();
    std::move(callback).Run({});
    return;
  }
  // Handle success. Url will be fetched in (browser process) caller.
  auto result = ai_chat::mojom::PageContent::New();
  result->content = mojom::PageContentData::NewContentUrl(transcript_url);
  result->type = std::move(type);
  std::move(callback).Run(std::move(result));
}

void PageContentExtractor::GetSearchSummarizerKey(
    mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) {
  auto element =
      render_frame()->GetWebFrame()->GetDocument().Head().QuerySelector(
          "meta[name=summarizer-key]");
  if (element.IsNull()) {
    std::move(callback).Run({});
    return;
  }
  std::move(callback).Run(element.GetAttribute("content").Utf8());
}

void PageContentExtractor::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  auto element = render_frame()->GetWebFrame()->GetDocument().GetElementById(
      "continue-with-leo");
  if (element.IsNull() || !element.HasHTMLTagName("a")) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  GURL url(element.GetAttribute("href").Utf8());
  std::string nonce = element.GetAttribute("data-nonce").Utf8();
  if (!IsOpenAIChatButtonFromBraveSearchURL(url) || nonce.empty() ||
      url.ref() != nonce) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(nonce);
}

}  // namespace ai_chat
