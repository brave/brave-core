// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/page_content_extractor.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/renderer/page_text_distilling.h"
#include "brave/components/ai_chat/renderer/yt_util.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "url/gurl.h"
#include "url/url_constants.h"
#include "v8/include/v8-isolate.h"

namespace ai_chat {

namespace {

constexpr char16_t kYoutubeTranscriptUrlExtractionScript[] =
    uR"JS(
      (function() {
        return ytplayer?.config?.args?.raw_player_response?.captions?.playerCaptionsTracklistRenderer?.captionTracks
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
  // When content has been pushed to this class from a throttle via
  // OnInterceptedPageContentChanged, use that content instead of fetching it
  // from the page.
  if (intercepted_content_) {
    auto intercepted_content = std::move(intercepted_content_);
    intercepted_content_.reset();
    DVLOG(1) << "Using intercepted content.";
    DCHECK_EQ(intercepted_content->type,
              InterceptedContentType::kYouTubeMetadataString)
        << "Unexpected intercepted content type";
    // Parse the YT metadata and extract the most appropriate caption Url
    auto maybe_caption_url =
        ParseAndChooseCaptionTrackUrl(intercepted_content->content);
    if (maybe_caption_url.has_value()) {
      GURL caption_url =
          render_frame()->GetWebFrame()->GetDocument().CompleteURL(
              blink::WebString::FromASCII(maybe_caption_url.value()));
      if (caption_url.is_valid()) {
        mojom::PageContentPtr content_update = mojom::PageContent::New();
        content_update->type = mojom::PageContentType::VideoTranscriptYouTube;
        content_update->content =
            mojom::PageContentData::NewContentUrl(caption_url);
        std::move(callback).Run(std::move(content_update));
        return;
      }
    }
    std::move(callback).Run({});
    return;
  }

  blink::WebLocalFrame* main_frame = render_frame()->GetWebFrame();
  GURL origin =
      url::Origin(((const blink::WebFrame*)main_frame)->GetSecurityOrigin())
          .GetURL();

  // Decide which technique to use to extract content from page...
  // 1) Video - YouTube's custom link to transcript
  // 2) Video - <track> element specifying text location
  // 3) Text - find the "main" text of the page

  if (origin.is_valid()) {
    std::string host = origin.host();
    if (base::Contains(kYouTubeHosts, host)) {
      VLOG(1) << "YouTube transcript type";
      // Do Youtube extraction
      v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
      blink::WebScriptSource source = blink::WebScriptSource(
          blink::WebString::FromUTF16(kYoutubeTranscriptUrlExtractionScript));
      auto script_callback = base::BindOnce(
          &PageContentExtractor::OnJSTranscriptUrlResult,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback),
          ai_chat::mojom::PageContentType::VideoTranscriptYouTube);
      // Main world so that we can access a global variable
      main_frame->RequestExecuteScript(
          global_world_id_, base::make_span(&source, 1u),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::EvaluationTiming::kAsynchronous,
          blink::mojom::LoadEventBlockingOption::kDoNotBlock,
          std::move(script_callback), blink::BackForwardCacheAware::kAllow,
          blink::mojom::WantResultOption::kWantResult,
          blink::mojom::PromiseResultOption::kAwait);
      return;
    } else if (base::Contains(kVideoTrackHosts, host)) {
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
          isolated_world_id_, base::make_span(&source, 1u),
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
      render_frame(), isolated_world_id_,
      base::BindOnce(&PageContentExtractor::OnDistillResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PageContentExtractor::OnInterceptedPageContentChanged(
    std::unique_ptr<AIChatResourceSnifferThrottleDelegate::InterceptedContent>
        content_update) {
  DCHECK_EQ(content_update->type,
            AIChatResourceSnifferThrottleDelegate::InterceptedContentType::
                kYouTubeMetadataString)
      << " - unexpected content type";
  DCHECK(!content_update->content.empty());

  // Store the new content for later, when we're asked for it, so that we
  // don't have to do any parsing or fetching when there's no active
  // conversation.
  intercepted_content_ = std::move(content_update);
  // Let the host know that new content was received so that it may record a
  // "page" change.
  mojo::AssociatedRemote<mojom::PageContentExtractorHost> host;
  render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&host);
  host->OnInterceptedPageContentChanged();
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

void PageContentExtractor::OnJSTranscriptUrlResult(
    ai_chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
    ai_chat::mojom::PageContentType type,
    std::optional<base::Value> value,
    base::TimeTicks start_time) {
  DVLOG(2) << "Video transcript Url extraction script completed and took"
           << (base::TimeTicks::Now() - start_time).InMillisecondsF() << "ms"
           << "\nResult: " << (value ? value->DebugString() : "[undefined]");

  // Handle no result from script
  if (!value.has_value()) {
    std::move(callback).Run({});
    return;
  }

  // Optional parsing
  std::string url;
  if (type == mojom::PageContentType::VideoTranscriptYouTube) {
    auto maybe_url = ChooseCaptionTrackUrl(value->GetIfList());
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

}  // namespace ai_chat
