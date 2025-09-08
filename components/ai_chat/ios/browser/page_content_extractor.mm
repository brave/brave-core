// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/ios/browser/page_content_extractor.h"

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
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/web_state.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char16_t kYoutubeInnerTubeConfigExtractionScript[] = uR"JS(
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

constexpr char16_t kPageDistillerScript[] = uR"JS(
      (function() {
          const kRolesToSkip = [
              "audio", "banner", "button", "complementary",
              "contentinfo", "footer", "img", "label", "navigation",
              "textbox", "combobox", "listbox", "checkbox", "radiobutton",
              "slider", "spinbutton", "searchbox"
          ];

          const kTagsToSkip = [
              "AUDIO", "HEADER", "BUTTON", "ASIDE",
              "FOOTER", "IMG", "PICTURE", "LABEL", "NAV",
              "INPUT", "BUTTON", "SEARCH", "STYLE", "SCRIPT"
          ];

          /**
           * Finds and concatenates all text nodes within a given root element,
           * while skipping specified tags and roles.
           * @param {Element} rootNode The element to start the traversal from.
           * @returns {string} The concatenated text from all found text nodes.
           */
          function extractTextFromRoot(rootNode) {
              let textContent = [];

              // Filter for both text and element nodes
              const treeWalker = document.createTreeWalker(
                  rootNode,
                  NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT,
                  {
                      acceptNode: function(node) {
                          const parent = node.parentElement;

                          if (node.nodeType === Node.ELEMENT_NODE &&
                              kTagsToSkip.includes(
                                node.tagName.toUpperCase())) {
                              return NodeFilter.FILTER_REJECT;
                          }

                          if (parent) {
                              if (kTagsToSkip.includes(
                                parent.tagName.toUpperCase())) {
                                  return NodeFilter.FILTER_REJECT;
                              }

                              if (parent.hasAttribute('role') &&
                                  kRolesToSkip.includes(
                                    parent.getAttribute('role'))) {
                                  return NodeFilter.FILTER_REJECT;
                              }
                          }

                          if (node.nodeType === Node.TEXT_NODE &&
                              node.nodeValue.trim() === '') {
                              return NodeFilter.FILTER_SKIP;
                          }

                          if (node.nodeType === Node.ELEMENT_NODE) {
                              return NodeFilter.FILTER_ACCEPT;
                          }

                          return NodeFilter.FILTER_ACCEPT;
                      }
                  }
              );

              let currentNode;
              while (currentNode = treeWalker.nextNode()) {
                  if (currentNode.nodeType === Node.TEXT_NODE) {
                      textContent.push(currentNode.nodeValue.trim());
                  }
              }

              return textContent.join(' ');
          }

          return extractTextFromRoot(document.body);
      })();
    )JS";

// TODO(petemill): Use heuristics to determine if page's main focus is
// a video, and not a hard-coded list of Url hosts.
constexpr auto kVideoTrackHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "www.ted.com",
                                             });

constexpr char16_t kGetMetaContentScript[] = uR"JS(document.head.querySelector(
          'meta[name=summarizer-key]')?.getAttribute('content'))JS";

const char16_t kLeoButtonNonceScript[] = uR"JS(
  (function() {
    const element = document.getElementById("continue-with-leo");
    if (!element || element.tagName.toLowerCase() !== "a") {
      return null;
    }

    const href = element.getAttribute("href");
    const nonce = element.getAttribute("data-nonce");

    const url = new URL(href, window.location.href);
    return {"href": url.href, "nonce": nonce};
  })();
)JS";

}  // namespace

PageContentExtractor::PageContentExtractor(web::WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);

  auto* web_frames_manager =
      web_state_->GetWebFramesManager(web::ContentWorld::kIsolatedWorld);
  if (web_frames_manager) {
    web_frames_manager->AddObserver(this);
  }

  // Bind mojom API to allow browser to communicate with this class
  web_state->GetInterfaceBinderForMainFrame()->AddInterface(base::BindRepeating(
      &PageContentExtractor::BindReceiver, base::Unretained(this)));
}

PageContentExtractor::~PageContentExtractor() {
  web_state_->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::PageContentExtractor::Name_);

  auto* web_frames_manager =
      web_state_->GetWebFramesManager(web::ContentWorld::kIsolatedWorld);
  if (web_frames_manager) {
    web_frames_manager->RemoveObserver(this);
  }

  web_state_->RemoveObserver(this);
}

void PageContentExtractor::WebFrameBecameAvailable(
    web::WebFramesManager* web_frames_manager,
    web::WebFrame* web_frame) {
  if (!web_frame->IsMainFrame()) {
    return;
  }

  this->main_frame_ = web_frame;
}

void PageContentExtractor::WebFrameBecameUnavailable(
    web::WebFramesManager* web_frames_manager,
    const std::string& frame_id) {
  if (this->main_frame_ && this->main_frame_->GetFrameId() == frame_id) {
    this->main_frame_ = nullptr;
  }
}

void PageContentExtractor::WebStateDestroyed(web::WebState* web_state) {
  delete this;
}

base::WeakPtr<PageContentExtractor> PageContentExtractor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PageContentExtractor::ExtractPageContent(
    mojom::PageContentExtractor::ExtractPageContentCallback callback) {
  VLOG(1) << "AI Chat renderer has been asked for page content.";

  if (!main_frame_) {
    main_frame_ =
        web_state_->GetWebFramesManager(web::ContentWorld::kIsolatedWorld)
            ->GetMainWebFrame();
  }

  if (!main_frame_) {
    std::move(callback).Run({});
    return;
  }

  GURL origin = url::Origin(main_frame_->GetSecurityOrigin()).GetURL();

  // Decide which technique to use to extract content from page...
  // 1) Video - YouTube's custom link to transcript
  // 2) Video - <track> element specifying text location
  // 3) Text - find the "main" text of the page

  if (origin.is_valid()) {
    std::string host = origin.host();
    if (kYouTubeHosts.contains(host)) {
      VLOG(1) << "YouTube transcript type";
      // Do Youtube extraction
      auto script_callback = base::BindOnce(
          &PageContentExtractor::OnJSYoutubeInnerTubeConfigResult,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback),
          ai_chat::mojom::PageContentType::VideoTranscriptYouTube);

      // Main world so that we can access a global variable
      ExecuteJavaScript(kYoutubeInnerTubeConfigExtractionScript,
                        std::move(script_callback));
      return;
    } else if (kVideoTrackHosts.contains(host)) {
      VLOG(1) << "Video track transcript type";
      // Video <track> extraction
      // TODO(petemill): Use heuristics to determine if page's main focus is
      // a video, and not a hard-coded list of Url hosts.
      auto script_callback = base::BindOnce(
          &PageContentExtractor::OnJSTranscriptUrlResult,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback),
          ai_chat::mojom::PageContentType::VideoTranscriptYouTube);

      // Main world so that we can access a global variable
      ExecuteJavaScript(kVideoTrackTranscriptUrlExtractionScript,
                        std::move(script_callback));
      return;
    }
  }

  VLOG(1) << "Text transcript type";

  // Do text extraction
  auto script_callback =
      base::BindOnce(&PageContentExtractor::OnDistillResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  ExecuteJavaScript(kPageDistillerScript, std::move(script_callback));
}

void PageContentExtractor::BindReceiver(
    mojo::PendingReceiver<mojom::PageContentExtractor> receiver) {
  VLOG(1) << "AIChat PageContentExtractor handler bound.";
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void PageContentExtractor::OnDistillResult(
    mojom::PageContentExtractor::ExtractPageContentCallback callback,
    std::optional<base::Value> content,
    base::TimeTicks start_time) {
  // Validate
  if (!content.has_value()) {
    VLOG(1) << "null content";
    std::move(callback).Run({});
    return;
  }

  if (auto* str = content->GetIfString()) {
    if (!str || str->empty()) {
      VLOG(1) << "Empty string";
      std::move(callback).Run({});
      return;
    }

    VLOG(1) << "Got a distill result of character length: " << str->length();
    // Successful text extraction
    auto result = mojom::PageContent::New();
    result->type = std::move(mojom::PageContentType::Text);
    result->content = mojom::PageContentData::NewContent(*str);
    std::move(callback).Run(std::move(result));
    return;
  }

  VLOG(1) << "Distilled content is not a string";
  std::move(callback).Run({});
  return;
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
  GURL transcript_url = web_state_->GetLastCommittedURL().Resolve(url);
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
  auto script_callback = base::BindOnce(
      [](decltype(callback) cb, std::optional<base::Value> content,
         base::TimeTicks start_time) {
        std::string* str = content ? content->GetIfString() : nullptr;
        if (str) {
          std::move(cb).Run(*str);
        } else {
          std::move(cb).Run({});
        }
      },
      std::move(callback));

  ExecuteJavaScript(kGetMetaContentScript, std::move(script_callback));
}

void PageContentExtractor::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  auto script_callback = base::BindOnce(
      [](decltype(callback) cb, std::optional<base::Value> result,
         base::TimeTicks start_time) {
        std::optional<std::string> nonce;

        if (!result || !result->is_dict()) {
          std::move(cb).Run(std::nullopt);
          return;
        }

        const base::Value::Dict& result_dict = result->GetDict();
        const std::string* href_str = result_dict.FindString("href");
        const std::string* nonce_str = result_dict.FindString("nonce");

        if (!href_str || !nonce_str) {
          std::move(cb).Run(std::nullopt);
          return;
        }

        GURL url(*href_str);
        if (!IsOpenAIChatButtonFromBraveSearchURL(url) || nonce_str->empty() ||
            url.ref_piece() != *nonce_str) {
          std::move(cb).Run(std::nullopt);
          return;
        }

        std::move(cb).Run(*nonce_str);
      },
      std::move(callback));

  ExecuteJavaScript(kLeoButtonNonceScript, std::move(script_callback));
}

void PageContentExtractor::ExecuteJavaScript(
    const std::u16string& script,
    base::OnceCallback<void(std::optional<base::Value>, base::TimeTicks)>
        callback) {
  auto script_callback = base::BindOnce(
      [](decltype(callback) cb, base::TimeTicks start_time,
         const base::Value* value) {
        std::move(cb).Run(
            value ? std::optional<base::Value>(value->Clone()) : std::nullopt,
            start_time);
      },
      std::move(callback), base::TimeTicks::Now());

  main_frame_->ExecuteJavaScript(script, std::move(script_callback));
}

}  // namespace ai_chat
