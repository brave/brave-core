// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"

#include <memory>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree_manager.h"

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
#include "brave/components/text_recognition/browser/text_recognition.h"
#endif

namespace ai_chat {

namespace {

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
// Hosts to use for screenshot based text retrieval
constexpr auto kScreenshotRetrievalHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "docs.google.com",
                                                 "twitter.com",
                                             });
#endif

constexpr auto kVideoPageContentTypes =
    base::MakeFixedFlatSet<ai_chat::mojom::PageContentType>(
        {ai_chat::mojom::PageContentType::VideoTranscriptYouTube,
         ai_chat::mojom::PageContentType::VideoTranscriptVTT});

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to fetch video transcript"
          "on behalf of the user interacting with the ChatUI."
        trigger:
          "Triggered by user asking for a summary."
        data:
          "Provided by the website that contains the video"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

class PageContentFetcher {
 public:
  void Start(mojo::Remote<mojom::PageContentExtractor> content_extractor,
             std::string_view invalidation_token,
             scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
             FetchPageContentCallback callback) {
    url_loader_factory_ = url_loader_factory;
    content_extractor_ = std::move(content_extractor);
    if (!content_extractor_) {
      DeleteSelf();
      return;
    }
    // Unretained is OK here. The `mojo::Remote` will not invoke callbacks
    // after it is destroyed.
    content_extractor_.set_disconnect_handler(base::BindOnce(
        &PageContentFetcher::DeleteSelf, base::Unretained(this)));
    content_extractor_->ExtractPageContent(base::BindOnce(
        &PageContentFetcher::OnTabContentResult, base::Unretained(this),
        std::move(callback), invalidation_token));
  }

 private:
  void DeleteSelf() { delete this; }

  void SendResultAndDeleteSelf(FetchPageContentCallback callback,
                               std::string content = "",
                               std::string invalidation_token = "",
                               bool is_video = false) {
    std::move(callback).Run(content, is_video, invalidation_token);
    delete this;
  }

  void OnTabContentResult(FetchPageContentCallback callback,
                          std::string_view invalidation_token,
                          mojom::PageContentPtr data) {
    if (!data) {
      VLOG(1) << __func__ << " no data.";
      SendResultAndDeleteSelf(std::move(callback));
      return;
    }
    DVLOG(1) << "OnTabContentResult: " << data.get();
    const bool is_video = base::Contains(kVideoPageContentTypes, data->type);
    DVLOG(1) << "Is video? " << is_video;
    // Handle text mode response
    if (!is_video) {
      DCHECK(data->content->is_content());
      auto content = data->content->get_content();
      DVLOG(1) << __func__ << ": Got content with char length of "
               << content.length();
      SendResultAndDeleteSelf(std::move(callback), content, "", false);
      return;
    }
    // If it's video, we expect content url
    DCHECK(data->content->is_content_url());
    auto content_url = data->content->get_content_url();
    if (content_url.is_empty() || !content_url.is_valid() ||
        !content_url.SchemeIs(url::kHttpsScheme)) {
      VLOG(1) << "Invalid content_url";
      SendResultAndDeleteSelf(std::move(callback), "", "", true);
      return;
    }
    // Subsequent calls do not need to re-fetch if the url stays the same
    auto new_invalidation_token = content_url.spec();
    if (new_invalidation_token == invalidation_token) {
      VLOG(2) << "Not fetching content since invalidation token matches: "
              << invalidation_token;
      SendResultAndDeleteSelf(std::move(callback), "", new_invalidation_token,
                              true);
      return;
    }
    DVLOG(1) << "Making video transcript fetch to " << content_url.spec();
    // Handle transcript url result - fetch content.
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = content_url;
    request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
    request->credentials_mode = network::mojom::CredentialsMode::kOmit;
    request->method = net::HttpRequestHeaders::kGetMethod;
    auto loader = network::SimpleURLLoader::Create(
        std::move(request), GetNetworkTrafficAnnotationTag());
    loader->SetRetryOptions(
        1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
               network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
    loader->SetAllowHttpErrorResults(true);
    auto* loader_ptr = loader.get();
    bool is_youtube =
        data->type == ai_chat::mojom::PageContentType::VideoTranscriptYouTube;
    auto on_response =
        base::BindOnce(&PageContentFetcher::OnTranscriptFetchResponse,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       std::move(loader), is_youtube, new_invalidation_token);
    loader_ptr->DownloadToString(url_loader_factory_.get(),
                                 std::move(on_response), 2 * 1024 * 1024);
  }

  void OnYoutubeTranscriptXMLParsed(
      FetchPageContentCallback callback,
      std::string invalidation_token,
      base::expected<base::Value, std::string> result) {
    // Example Youtube transcript XML:
    //
    // <?xml version="1.0" encoding="utf-8"?>
    // <transcript>
    //   <text start="0" dur="5.1">Dear Fellow Scholars, this is Two Minute
    //   Papers with Dr. Károly Zsolnai-Fehér.</text> <text start="5.1"
    //   dur="5.28">ChatGPT has just been supercharged with  browsing support, I
    //   tried it myself too,  </text> <text start="10.38" dur="7.38">and I
    //   think this changes everything. Well, almost  everything, as you will
    //   see in a moment. And this  </text>
    // </transcript>

    if (!data_decoder::IsXmlElementNamed(result.value(), "transcript")) {
      VLOG(1) << "Could not find transcript element.";
      return;
    }

    std::string transcript_text;
    const base::Value::List* children =
        data_decoder::GetXmlElementChildren(result.value());
    if (!children) {
      return;
    }

    for (const auto& child : *children) {
      if (!data_decoder::IsXmlElementNamed(child, "text")) {
        continue;
      }

      std::string text;
      if (!data_decoder::GetXmlElementText(child, &text)) {
        continue;
      }

      if (!transcript_text.empty()) {
        // Add a space as a separator betwen texts.
        transcript_text += " ";
      }

      transcript_text += text;
    }

    SendResultAndDeleteSelf(std::move(callback), transcript_text,
                            invalidation_token, true);
  }

  void OnTranscriptFetchResponse(
      FetchPageContentCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      bool is_youtube,
      std::string invalidation_token,
      std::unique_ptr<std::string> response_body) {
    auto response_code = -1;
    base::flat_map<std::string, std::string> headers;
    if (loader->ResponseInfo()) {
      auto headers_list = loader->ResponseInfo()->headers;
      if (headers_list) {
        response_code = headers_list->response_code();
      }
    }

    // Validate if we get a feed
    bool is_ok = (loader->NetError() == net::OK && response_body);
    std::string transcript_content = is_ok ? *response_body : "";
    if (!is_ok || transcript_content.empty()) {
      DVLOG(1) << __func__ << " invalid video transcript response from url: "
               << loader->GetFinalURL().spec() << " status: " << response_code;
    }
    DVLOG(2) << "Got video text: " << transcript_content;
    VLOG(1) << __func__ << " Number of chars in video transcript xml = "
            << transcript_content.length() << "\n";
    if (is_youtube) {
      data_decoder::DataDecoder::ParseXmlIsolated(
          transcript_content,
          data_decoder::mojom::XmlParser::WhitespaceBehavior::
              kPreserveSignificant,
          base::BindOnce(&PageContentFetcher::OnYoutubeTranscriptXMLParsed,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                         invalidation_token));
      return;
    }

    SendResultAndDeleteSelf(std::move(callback), transcript_content,
                            invalidation_token, true);
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::Remote<mojom::PageContentExtractor> content_extractor_;
  base::WeakPtrFactory<PageContentFetcher> weak_ptr_factory_{this};
};

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void OnGetTextFromImage(
    FetchPageContentCallback callback,
    const std::pair<bool, std::vector<std::string>>& supported_strs) {
  if (!supported_strs.first) {
    std::move(callback).Run("", false, "");
    return;
  }

  std::stringstream ss;
  auto& strs = supported_strs.second;
  for (size_t i = 0; i < strs.size(); ++i) {
    ss << base::TrimWhitespaceASCII(strs[i], base::TrimPositions::TRIM_ALL);
    if (i < strs.size() - 1) {
      ss << "\n";
    }
  }
  std::move(callback).Run(ss.str(), false, "");
}

void OnScreenshot(FetchPageContentCallback callback, const SkBitmap& image) {
#if BUILDFLAG(IS_MAC)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&text_recognition::GetTextFromImage, image),
      base::BindOnce(&OnGetTextFromImage, std::move(callback)));
#endif
#if BUILDFLAG(IS_WIN)
  const std::string& locale = brave_l10n::GetDefaultLocaleString();
  const std::string language_code = brave_l10n::GetISOLanguageCode(locale);
  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTask(FROM_HERE,
                 base::BindOnce(
                     &text_recognition::GetTextFromImage, language_code, image,
                     base::BindPostTaskToCurrentDefault(base::BindOnce(
                         &OnGetTextFromImage, std::move(callback)))));

#endif
}
#endif  // #if BUILDFLAG(ENABLE_TEXT_RECOGNITION)

}  // namespace

void FetchPageContent(content::WebContents* web_contents,
                      std::string_view invalidation_token,
                      FetchPageContentCallback callback) {
  VLOG(2) << __func__ << " Extracting page content from renderer...";

  auto* primary_rfh = web_contents->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  if (!primary_rfh) {
    LOG(ERROR)
        << "Content extraction request submitted for a WebContents without "
           "a primary main frame";
    std::move(callback).Run("", false, "");
    return;
  }

  if (web_contents->GetContentsMimeType() == "application/pdf") {
    ui::AXTreeID ax_tree_id;
    // FindPdfChildFrame
    primary_rfh->ForEachRenderFrameHost(
        [&ax_tree_id](content::RenderFrameHost* rfh) {
          if (!rfh->GetProcess()->IsPdf()) {
            return;
          }
          ax_tree_id = rfh->GetAXTreeID();
        });
    if (ax_tree_id.type() != ax::mojom::AXTreeIDType::kUnknown) {
      auto* ax_tree_manager = ui::AXTreeManager::FromID(ax_tree_id);
      if (ax_tree_manager) {
        auto* ax_node = ax_tree_manager->GetRoot();
        auto pdf_content = ax_node->GetTextContentUTF8();
        if (!pdf_content.empty()) {
          std::move(callback).Run(pdf_content, false, "");
          return;
        }
      }
    }
    // No need to proceed renderer content fetching because we won't get any.
    std::move(callback).Run("", false, "");
    return;
  }

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  auto host = web_contents->GetURL().host();
  if (base::Contains(kScreenshotRetrievalHosts, host)) {
    content::RenderWidgetHostView* view =
        web_contents->GetRenderWidgetHostView();
    if (view) {
      gfx::Size content_size = web_contents->GetSize();
      gfx::Rect capture_area(0, 0, content_size.width(), content_size.height());
      view->CopyFromSurface(capture_area, content_size,
                            base::BindOnce(&OnScreenshot, std::move(callback)));
      return;
    }
  }
#endif

  mojo::Remote<mojom::PageContentExtractor> extractor;

  // GetRemoteInterfaces() cannot be null if the render frame is created.
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());

  auto* fetcher = new PageContentFetcher();
  auto* loader = web_contents->GetBrowserContext()
                     ->GetDefaultStoragePartition()
                     ->GetURLLoaderFactoryForBrowserProcess()
                     .get();
  fetcher->Start(std::move(extractor), invalidation_token, loader,
                 std::move(callback));
}

}  // namespace ai_chat
