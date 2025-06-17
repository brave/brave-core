// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/core/common/yt_util.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"
#include "services/data_decoder/public/mojom/xml_parser.mojom.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
#include "brave/components/ai_chat/core/browser/utils.h"
#include "content/public/browser/render_widget_host_view.h"
#endif  // BUILDFLAG(ENABLE_TEXT_RECOGNITION)

namespace ai_chat {

namespace {

using FetchPageContentCallback =
    AIChatTabHelper::PageContentFetcherDelegate::FetchPageContentCallback;

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
// Hosts to use for screenshot based text retrieval
constexpr auto kScreenshotRetrievalHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "twitter.com",
                                             });
#endif

constexpr auto kVideoPageContentTypes =
    base::MakeFixedFlatSet<ai_chat::mojom::PageContentType>(
        {ai_chat::mojom::PageContentType::VideoTranscriptYouTube,
         ai_chat::mojom::PageContentType::VideoTranscriptVTT});

net::NetworkTrafficAnnotationTag GetVideoNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_video", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to fetch video transcript"
          "on behalf of the user interacting with the ChatUI."
        trigger:
          "Triggered by user communicating with Leo"
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

net::NetworkTrafficAnnotationTag GetGithubNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_github", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to fetch github pull request patch files "
          "on behalf of the user when interacting with Leo on github."
        trigger:
          "Triggered by user communicating with Leo on github.com"
        data:
          "Provided by github"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: YES
        policy_exception_justification: "Cookies necessary for private repos."
      }
    )");
}

class PageContentFetcherInternal {
 public:
  explicit PageContentFetcherInternal(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : url_loader_factory_(url_loader_factory) {}

  void Start(mojo::Remote<mojom::PageContentExtractor> content_extractor,
             std::string_view invalidation_token,
             FetchPageContentCallback callback) {
    content_extractor_ = std::move(content_extractor);
    if (!content_extractor_) {
      DeleteSelf();
      return;
    }
    // Unretained is OK here. The `mojo::Remote` will not invoke callbacks
    // after it is destroyed.
    content_extractor_.set_disconnect_handler(base::BindOnce(
        &PageContentFetcherInternal::DeleteSelf, base::Unretained(this)));
    content_extractor_->ExtractPageContent(base::BindOnce(
        &PageContentFetcherInternal::OnTabContentResult, base::Unretained(this),
        std::move(callback), invalidation_token));
  }

  void GetSearchSummarizerKey(
      mojo::Remote<mojom::PageContentExtractor> content_extractor,
      mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) {
    content_extractor_ = std::move(content_extractor);
    if (!content_extractor_) {
      DeleteSelf();
      return;
    }
    content_extractor_.set_disconnect_handler(base::BindOnce(
        &PageContentFetcherInternal::DeleteSelf, base::Unretained(this)));
    content_extractor_->GetSearchSummarizerKey(std::move(callback));
  }

  void GetOpenAIChatButtonNonce(
      mojo::Remote<mojom::PageContentExtractor> content_extractor,
      mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
    content_extractor_ = std::move(content_extractor);
    if (!content_extractor_) {
      DeleteSelf();
      return;
    }
    content_extractor_.set_disconnect_handler(base::BindOnce(
        &PageContentFetcherInternal::DeleteSelf, base::Unretained(this)));
    content_extractor_->GetOpenAIChatButtonNonce(std::move(callback));
  }

  void StartGithub(GURL patch_url, FetchPageContentCallback callback) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = patch_url;
    request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
    request->credentials_mode = network::mojom::CredentialsMode::kInclude;
    request->site_for_cookies =
        net::SiteForCookies::FromOrigin(url::Origin::Create(patch_url));
    request->method = net::HttpRequestHeaders::kGetMethod;
    auto loader = network::SimpleURLLoader::Create(
        std::move(request), GetGithubNetworkTrafficAnnotationTag());
    loader->SetRetryOptions(
        1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
               network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
    loader->SetAllowHttpErrorResults(true);
    auto* loader_ptr = loader.get();
    auto on_response = base::BindOnce(
        &PageContentFetcherInternal::OnPatchFetchResponse,
        weak_ptr_factory_.GetWeakPtr(), std::move(callback), std::move(loader));
    loader_ptr->DownloadToString(url_loader_factory_.get(),
                                 std::move(on_response), 2 * 1024 * 1024);
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
    const bool is_video = kVideoPageContentTypes.contains(data->type);
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
    if (data->content->is_content_url()) {
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
          std::move(request), GetVideoNetworkTrafficAnnotationTag());
      loader->SetRetryOptions(
          1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
                 network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
      loader->SetAllowHttpErrorResults(true);
      auto* loader_ptr = loader.get();
      bool is_youtube =
          data->type == ai_chat::mojom::PageContentType::VideoTranscriptYouTube;
      auto on_response =
          base::BindOnce(&PageContentFetcherInternal::OnTranscriptFetchResponse,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                         std::move(loader), is_youtube, new_invalidation_token);
      loader_ptr->DownloadToString(url_loader_factory_.get(),
                                   std::move(on_response), 2 * 1024 * 1024);
    } else if (data->content->is_youtube_inner_tube_config()) {
      const auto& config = data->content->get_youtube_inner_tube_config();
      DVLOG(1) << "Making InnerTube API request for video " << config->video_id;

      auto url = GURL(base::StringPrintf(
          "https://www.youtube.com/youtubei/v1/player?key=%s",
          config->api_key.c_str()));
      auto new_invalidation_token = url.spec();
      if (new_invalidation_token == invalidation_token) {
        VLOG(2) << "Not fetching content since invalidation token matches: "
                << invalidation_token;
        SendResultAndDeleteSelf(std::move(callback), "", new_invalidation_token,
                                true);
        return;
      }
      // Make InnerTube API request
      auto request = std::make_unique<network::ResourceRequest>();
      request->url = url;
      request->method = net::HttpRequestHeaders::kPostMethod;
      request->headers.SetHeader(net::HttpRequestHeaders::kContentType,
                                 "application/json");
      request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
      request->credentials_mode = network::mojom::CredentialsMode::kOmit;

      auto loader = network::SimpleURLLoader::Create(
          std::move(request), GetVideoNetworkTrafficAnnotationTag());

      // Set up request body
      base::Value::Dict body;
      body.Set("videoId", config->video_id);
      base::Value::Dict context;
      base::Value::Dict client;
      client.Set("clientName", "ANDROID");
      client.Set("clientVersion", "20.10.38");
      context.Set("client", std::move(client));
      body.Set("context", std::move(context));

      std::string body_str;
      base::JSONWriter::Write(body, &body_str);
      loader->AttachStringForUpload(body_str, "application/json");

      auto* loader_ptr = loader.get();
      auto on_response = base::BindOnce(
          &PageContentFetcherInternal::OnInnerTubePlayerJsonResponse,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback),
          std::move(loader), new_invalidation_token);

      loader_ptr->DownloadToString(url_loader_factory_.get(),
                                   std::move(on_response), 2 * 1024 * 1024);
    }
  }

  void OnTranscriptFetchResponse(
      FetchPageContentCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      bool is_youtube,
      std::string invalidation_token,
      std::unique_ptr<std::string> response_body) {
    auto response_code = -1;
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
          base::BindOnce(
              &PageContentFetcherInternal::OnYoutubeTranscriptXMLParsed,
              weak_ptr_factory_.GetWeakPtr(), std::move(callback),
              invalidation_token));
      return;
    }

    SendResultAndDeleteSelf(std::move(callback), transcript_content,
                            invalidation_token, true);
  }

  void OnPatchFetchResponse(FetchPageContentCallback callback,
                            std::unique_ptr<network::SimpleURLLoader> loader,
                            std::unique_ptr<std::string> response_body) {
    auto response_code = -1;
    if (loader->ResponseInfo()) {
      auto headers_list = loader->ResponseInfo()->headers;
      if (headers_list) {
        response_code = headers_list->response_code();
      }
    }
    bool is_ok =
        loader->NetError() == net::OK && response_body && response_code == 200;

    std::string patch_content = is_ok ? *response_body : "";
    if (!is_ok || patch_content.empty()) {
      DVLOG(1) << __func__ << " invalid patch response from url: "
               << loader->GetFinalURL().spec() << " status: " << response_code;
    }
    DVLOG(2) << "Got patch: " << patch_content;
    SendResultAndDeleteSelf(std::move(callback), patch_content, "", false);
  }

  void OnInnerTubePlayerJsonResponse(
      FetchPageContentCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      std::string invalidation_token,
      std::unique_ptr<std::string> response_body) {
    if (!response_body || loader->NetError() != net::OK) {
      SendResultAndDeleteSelf(std::move(callback), "", invalidation_token,
                              true);
      return;
    }
    // Sanitize the response body
    api_request_helper::ParseJsonNonBlocking(
        *response_body,
        base::BindOnce(&PageContentFetcherInternal::OnInnerTubePlayerJsonParsed,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       invalidation_token));
  }

  void OnInnerTubePlayerJsonParsed(
      FetchPageContentCallback callback,
      std::string invalidation_token,
      base::expected<base::Value, std::string> result) {
    if (!result.has_value() || !result->is_dict()) {
      SendResultAndDeleteSelf(std::move(callback), "", invalidation_token,
                              true);
      return;
    }
    auto* caption_tracks = result->GetDict().FindListByDottedPath(
        "captions.playerCaptionsTracklistRenderer.captionTracks");
    if (!caption_tracks) {
      SendResultAndDeleteSelf(std::move(callback), "", invalidation_token,
                              true);
      return;
    }
    auto base_url = ai_chat::ChooseCaptionTrackUrl(*caption_tracks);
    if (!base_url) {
      SendResultAndDeleteSelf(std::move(callback), "", invalidation_token,
                              true);
      return;
    }
    DVLOG(1) << "Fetching transcript from baseUrl: " << *base_url;
    // Now fetch the transcript from baseUrl
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = GURL(*base_url);
    request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
    request->credentials_mode = network::mojom::CredentialsMode::kOmit;
    request->method = net::HttpRequestHeaders::kGetMethod;
    auto transcript_loader = network::SimpleURLLoader::Create(
        std::move(request), GetVideoNetworkTrafficAnnotationTag());
    transcript_loader->SetRetryOptions(
        1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
               network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
    transcript_loader->SetAllowHttpErrorResults(true);

    auto* loader_ptr = transcript_loader.get();
    auto on_response =
        base::BindOnce(&PageContentFetcherInternal::OnTranscriptFetchResponse,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       std::move(transcript_loader), true, invalidation_token);

    loader_ptr->DownloadToString(url_loader_factory_.get(),
                                 std::move(on_response), 2 * 1024 * 1024);
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

  void OnYoutubeTranscriptXMLParsed(
      FetchPageContentCallback callback,
      std::string invalidation_token,
      base::expected<base::Value, std::string> result) {
    if (!result.has_value()) {
      SendResultAndDeleteSelf(std::move(callback), "", invalidation_token,
                              true);
      return;
    }

    const base::Value& root = result.value();
    std::string transcript_text;

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
    if (data_decoder::IsXmlElementNamed(root, "transcript")) {
      // Existing YouTube transcript XML logic
      const base::Value::List* children =
          data_decoder::GetXmlElementChildren(root);
      if (!children) {
        SendResultAndDeleteSelf(std::move(callback), transcript_text,
                                invalidation_token, true);
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
          transcript_text += " ";
        }
        transcript_text += text;
      }
    } else if (data_decoder::IsXmlElementNamed(root, "timedtext")) {
      // Handle <timedtext> format ex.
      // <timedtext format="3">
      // <head>
      // <ws id="0"/>
      // <ws id="1" mh="2" ju="0" sd="3"/>
      // <wp id="0"/>
      // <wp id="1" ap="6" ah="20" av="100" rc="2" cc="40"/>
      // </head>
      // <body>
      // <w t="0" id="1" wp="1" ws="1"/>
      // <p t="160" d="4080" w="1">
      // <s ac="0">hi</s>
      // <s t="160" ac="0"> everyone</s>
      // <s t="1120" ac="0"> so</s>
      // <s t="1320" ac="0"> recently</s>
      // <s t="1720" ac="0"> I</s>
      // <s t="1840" ac="0"> gave</s>
      // <s t="1999" ac="0"> a</s>
      // </p>
      // <p t="2270" d="1970" w="1" a="1"> </p>
      // <p t="2280" d="4119" w="1">
      // <s ac="0">30-minute</s>
      // <s t="520" ac="0"> talk</s>
      // <s t="720" ac="0"> on</s>
      // <s t="879" ac="0"> large</s>
      // <s t="1159" ac="0"> language</s>
      // <s t="1480" ac="0"> models</s>
      // </p>
      // <p t="4230" d="2169" w="1" a="1"> </p>
      // ...
      // <p t="3585359" d="4321" w="1">
      // <s ac="0">keep</s>
      // <s t="161" ac="0"> track</s>
      // <s t="440" ac="0"> of</s>
      // <s t="1440" ac="0"> bye</s>
      // </p>
      // </body>
      // </timedtext>
      // or
      // <timedtext format="3">
      // <body>
      // <p t="13460" d="2175">Chris Anderson: This is such a strange thing.</p>
      // <p t="15659" d="3158">Your software, Linux, is in millions of
      // computers,</p>
      // <p t="18841" d="3547">it probably powers much of the Internet.</p>
      // <p t="22412" d="1763">And I think that there are, like,</p>
      // <p t="24199" d="3345">a billion and a half active Android devices out
      // there.</p>
      // <p t="27568" d="2601">Your software is in every single one of them.</p>
      // <p t="30808" d="1150">It's kind of amazing.</p>
      // <p t="31982" d="5035">You must have some amazing software headquarters
      // driving all this.</p>
      // <p t="37041" d="3306">That's what I thought -- and I was shocked when I
      // saw a picture of it.</p>
      // <p t="40371" d="1200">I mean, this is --</p>
      // <p t="41595" d="2250">this is the Linux world headquarters.</p>
      // ...
      // </body>
      // </timedtext>
      const base::Value::List* children =
          data_decoder::GetXmlElementChildren(root);
      if (!children) {
        SendResultAndDeleteSelf(std::move(callback), transcript_text,
                                invalidation_token, true);
        return;
      }
      for (const auto& child : *children) {
        if (!data_decoder::IsXmlElementNamed(child, "body")) {
          continue;
        }
        const base::Value::List* body_children =
            data_decoder::GetXmlElementChildren(child);
        if (!body_children) {
          continue;
        }
        for (const auto& p : *body_children) {
          if (!data_decoder::IsXmlElementNamed(p, "p")) {
            continue;
          }

          const base::Value::List* s_children =
              data_decoder::GetXmlElementChildren(p);
          bool all_s =
              s_children &&
              std::ranges::all_of(*s_children, [](const base::Value& s) {
                return data_decoder::IsXmlElementNamed(s, "s");
              });

          if (all_s && s_children && !s_children->empty()) {
            // All children are <s>
            for (const auto& s : *s_children) {
              std::string s_text;
              if (data_decoder::GetXmlElementText(s, &s_text)) {
                transcript_text += s_text;
              }
            }
          } else {
            // Not all children are <s>, so treat as direct text
            std::string p_text;
            if (data_decoder::GetXmlElementText(p, &p_text)) {
              transcript_text += p_text;
            }
          }
          transcript_text += " ";  // Space between <p> blocks
        }
      }
    }

    SendResultAndDeleteSelf(std::move(callback), transcript_text,
                            invalidation_token, true);
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  mojo::Remote<mojom::PageContentExtractor> content_extractor_;
  base::WeakPtrFactory<PageContentFetcherInternal> weak_ptr_factory_{this};
};

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void OnScreenshot(FetchPageContentCallback callback, const SkBitmap& image) {
  GetOCRText(image,
             base::BindOnce(
                 [](FetchPageContentCallback callback, std::string text) {
                   std::move(callback).Run(std::move(text), false, "");
                 },
                 std::move(callback)));
}
#endif  // #if BUILDFLAG(ENABLE_TEXT_RECOGNITION)

// Obtains a patch URL from a pull request URL
std::optional<GURL> GetGithubPatchURLForPRURL(const GURL& url) {
  if (!url.is_valid() || url.scheme() != "https" ||
      url.host() != "github.com") {
    return std::nullopt;
  }

  std::vector<std::string> path_parts = base::SplitString(
      url.path(), "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  // Only handle URLs of the form: /<user>/<repo>/pull/<number>
  if (path_parts.size() < 4 || path_parts[2] != "pull") {
    return std::nullopt;
  }
  std::string patch_url_str = url.GetWithEmptyPath().spec() + path_parts[0] +
                              "/" + path_parts[1] + "/pull/" + path_parts[3] +
                              ".patch";
  return GURL(patch_url_str);
}

}  // namespace

PageContentFetcher::PageContentFetcher(content::WebContents* web_contents)
    : web_contents_(web_contents),
      url_loader_factory_(web_contents_->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess()
                              .get()) {}

PageContentFetcher::~PageContentFetcher() = default;

void PageContentFetcher::FetchPageContent(std::string_view invalidation_token,
                                          FetchPageContentCallback callback) {
  VLOG(2) << __func__ << " Extracting page content from renderer...";

  auto* primary_rfh = web_contents_->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  if (!primary_rfh) {
    LOG(ERROR)
        << "Content extraction request submitted for a WebContents without "
           "a primary main frame";
    std::move(callback).Run("", false, "");
    return;
  }

  auto url = web_contents_->GetLastCommittedURL();
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  if (kScreenshotRetrievalHosts.contains(url.host_piece())) {
    content::RenderWidgetHostView* view =
        web_contents_->GetRenderWidgetHostView();
    if (view) {
      gfx::Size content_size = web_contents_->GetSize();
      gfx::Rect capture_area(0, 0, content_size.width(), content_size.height());
      view->CopyFromSurface(capture_area, content_size,
                            base::BindOnce(&OnScreenshot, std::move(callback)));
      return;
    }
  }
#endif
  auto* loader = url_loader_factory_.get();
  auto* fetcher = new PageContentFetcherInternal(loader);
  auto patch_url = GetGithubPatchURLForPRURL(url);
  if (patch_url) {
    fetcher->StartGithub(patch_url.value(), std::move(callback));
    return;
  }

  mojo::Remote<mojom::PageContentExtractor> extractor;
  // GetRemoteInterfaces() cannot be null if the render frame is created.
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());
  fetcher->Start(std::move(extractor), invalidation_token, std::move(callback));
}

void PageContentFetcher::GetSearchSummarizerKey(
    mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) {
  auto* primary_rfh = web_contents_->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  auto* fetcher = new PageContentFetcherInternal(nullptr);
  mojo::Remote<mojom::PageContentExtractor> extractor;
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());
  fetcher->GetSearchSummarizerKey(std::move(extractor), std::move(callback));
}

void PageContentFetcher::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  auto* primary_rfh = web_contents_->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  auto* fetcher = new PageContentFetcherInternal(nullptr);
  mojo::Remote<mojom::PageContentExtractor> extractor;
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());
  fetcher->GetOpenAIChatButtonNonce(std::move(extractor), std::move(callback));
}

}  // namespace ai_chat
