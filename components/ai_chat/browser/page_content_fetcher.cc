// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/browser/page_content_fetcher.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "brave/components/ai_chat/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/browser_context.h"
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

namespace ai_chat {

namespace {

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
    content_extractor_->ExtractPageContent(
        base::BindOnce(&PageContentFetcher::OnTabContentResult,
                       base::Unretained(this), std::move(callback)));
  }

 private:
  void DeleteSelf() { delete this; }

  void SendResultAndDeleteSelf(FetchPageContentCallback callback,
                               std::string content = "",
                               bool is_video = false) {
    std::move(callback).Run(content, is_video);
    delete this;
  }

  void OnTabContentResult(FetchPageContentCallback callback,
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
      SendResultAndDeleteSelf(std::move(callback), content, false);
      return;
    }
    // If it's video, we expect content url
    DCHECK(data->content->is_content_url());
    auto content_url = data->content->get_content_url();
    if (content_url.is_empty() || !content_url.is_valid() ||
        !content_url.SchemeIs(url::kHttpsScheme)) {
      VLOG(1) << "Invalid content_url";
      SendResultAndDeleteSelf(std::move(callback), "", true);
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
                       std::move(loader), is_youtube);
    loader_ptr->DownloadToString(url_loader_factory_.get(),
                                 std::move(on_response), 2 * 1024 * 1024);
  }

  void OnYoutubeTranscriptXMLParsed(
      FetchPageContentCallback callback,
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

    SendResultAndDeleteSelf(std::move(callback), transcript_text, true);
  }

  void OnTranscriptFetchResponse(
      FetchPageContentCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      bool is_youtube,
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
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
      return;
    }

    SendResultAndDeleteSelf(std::move(callback), transcript_content, true);
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::Remote<mojom::PageContentExtractor> content_extractor_;
  base::WeakPtrFactory<PageContentFetcher> weak_ptr_factory_{this};
};

}  // namespace

void FetchPageContent(content::WebContents* web_contents,
                      FetchPageContentCallback callback) {
  VLOG(2) << __func__ << " Extracting page content from renderer...";

  auto* primary_rfh = web_contents->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  if (!primary_rfh) {
    LOG(ERROR)
        << "Content extraction request submitted for a WebContents without "
           "a primary main frame";
    std::move(callback).Run("", false);
    return;
  }

  mojo::Remote<mojom::PageContentExtractor> extractor;

  // GetRemoteInterfaces() cannot be null if the render frame is created.
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());

  auto* fetcher = new PageContentFetcher();
  auto* loader = web_contents->GetBrowserContext()
                     ->GetDefaultStoragePartition()
                     ->GetURLLoaderFactoryForBrowserProcess()
                     .get();
  fetcher->Start(std::move(extractor), loader, std::move(callback));
}

}  // namespace ai_chat
