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
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/core/common/yt_util.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
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
#include "services/data_decoder/public/mojom/xml_parser.mojom.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
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
    AssociatedContentDriver::FetchPageContentCallback;

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
        &PageContentFetcherInternal::OnGithubContentFetchResponse,
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
    // If it's non YouTube video, we expect content url
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

      auto url = GURL(
          absl::StrFormat("https://www.youtube.com/youtubei/v1/player?key=%s",
                          base::EscapeQueryParamValue(config->api_key, true)));
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

  void OnGithubContentFetchResponse(
      FetchPageContentCallback callback,
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

    std::string content = is_ok ? *response_body : "";
    if (!is_ok || content.empty()) {
      DVLOG(1) << __func__ << " invalid content response from url: "
               << loader->GetFinalURL().spec() << " status: " << response_code;
    }
    DVLOG(2) << "Got content: " << content;
    SendResultAndDeleteSelf(std::move(callback), content, "", false);
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
    std::string transcript_text = ParseYoutubeTranscriptXml(root);
    SendResultAndDeleteSelf(std::move(callback), transcript_text,
                            invalidation_token, true);
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  mojo::Remote<mojom::PageContentExtractor> content_extractor_;
  base::WeakPtrFactory<PageContentFetcherInternal> weak_ptr_factory_{this};
};

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void OnScreenshot(FetchPageContentCallback callback,
                  const viz::CopyOutputBitmapWithMetadata& result) {
  const SkBitmap& bitmap = result.bitmap;
  GetOCRText(bitmap,
             base::BindOnce(
                 [](FetchPageContentCallback callback, std::string text) {
                   std::move(callback).Run(std::move(text), false, "");
                 },
                 std::move(callback)));
}
#endif  // #if BUILDFLAG(ENABLE_TEXT_RECOGNITION)

// Obtains a content URL from a GitHub URL (pull request, commit, compare,
// commits, or file blob)
std::optional<GURL> GetGithubContentURL(const GURL& url) {
  if (!url.is_valid() || url.scheme() != "https" ||
      url.host() != "github.com") {
    return std::nullopt;
  }

  std::vector<std::string> path_parts = base::SplitString(
      url.path(), "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (path_parts.size() < 4) {
    return std::nullopt;
  }

  const std::string& user = path_parts[0];
  const std::string& repo = path_parts[1];
  const std::string& type = path_parts[2];

  // Handle pull requests: /<user>/<repo>/pull/<number>
  if (type == "pull") {
    return GURL(base::StrCat({url.GetWithEmptyPath().spec(), user, "/", repo,
                              "/pull/", path_parts[3], ".patch"}));
  }

  // Handle commits: /<user>/<repo>/commit/<hash>
  if (type == "commit") {
    return GURL(base::StrCat({url.GetWithEmptyPath().spec(), user, "/", repo,
                              "/commit/", path_parts[3], ".patch"}));
  }

  // Handle compare: /<user>/<repo>/compare/<comparison>
  if (type == "compare") {
    return GURL(base::StrCat({url.GetWithEmptyPath().spec(), user, "/", repo,
                              "/compare/", path_parts[3], ".patch"}));
  }

  // Handle commits: /<user>/<repo>/commits/<branch>
  if (type == "commits") {
    return GURL(base::StrCat({url.GetWithEmptyPath().spec(), user, "/", repo,
                              "/commits/", path_parts[3], ".atom"}));
  }

  // Handle blob (file view): /<user>/<repo>/blob/<branch>/<path>
  if (type == "blob") {
    GURL::Replacements replacements;
    replacements.SetQueryStr("raw=true");
    return url.ReplaceComponents(replacements);
  }

  return std::nullopt;
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
  if (kScreenshotRetrievalHosts.contains(url.host())) {
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
  auto github_content_url = GetGithubContentURL(url);
  if (github_content_url) {
    DVLOG(2) << "Github url: " << *github_content_url;
    fetcher->StartGithub(github_content_url.value(), std::move(callback));
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
