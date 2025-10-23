// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/sanitized_image_source.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "base/containers/contains.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "components/image_fetcher/ios/ios_image_decoder_impl.h"
#include "components/signin/public/identity_manager/primary_account_access_token_fetcher.h"
#include "components/signin/public/identity_manager/scope_set.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/signin/model/identity_manager_factory.h"
#include "ios/web/public/browser_state.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "services/data_decoder/public/mojom/image_decoder.mojom.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/encode/SkWebpEncoder.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/codec/webp_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "url/url_util.h"

namespace {

// const int64_t kMaxImageSizeInBytes =
//     static_cast<int64_t>(134217728U);

constexpr char kEncodeTypeKey[] = "encodeType";
constexpr char kIsGooglePhotosKey[] = "isGooglePhotos";
constexpr char kStaticEncodeKey[] = "staticEncode";
constexpr char kUrlKey[] = "url";

std::map<std::string, std::string> ParseParams(std::string_view param_string) {
  url::Component query(0, param_string.size());
  url::Component key;
  url::Component value;
  constexpr int kMaxUriDecodeLen = 2048;
  std::map<std::string, std::string> params;
  while (url::ExtractQueryKeyValue(param_string, &query, &key, &value)) {
    url::RawCanonOutputW<kMaxUriDecodeLen> output;
    url::DecodeURLEscapeSequences(param_string.substr(value.begin, value.len),
                                  url::DecodeURLMode::kUTF8OrIsomorphic,
                                  &output);
    params.insert({std::string(param_string.substr(key.begin, key.len)),
                   base::UTF16ToUTF8(output.view())});
  }
  return params;
}

bool IsGooglePhotosUrl(const GURL& url) {
  static const char* const kGooglePhotosHostSuffixes[] = {
      ".ggpht.com",
      ".google.com",
      ".googleusercontent.com",
  };

  for (const char* const suffix : kGooglePhotosHostSuffixes) {
    if (base::EndsWith(url.host(), suffix)) {
      return true;
    }
  }
  return false;
}

}  // namespace

namespace chrome {
inline constexpr char kChromeUIImageHost[] = "image";
inline constexpr char kChromeUIImageURL[] = "chrome://image/";
}  // namespace chrome

void SanitizedImageSource::DataDecoderDelegate::DecodeImage(
    const std::string& data,
    DecodeImageCallback callback) {
  NSData* image_data = [NSData dataWithBytes:data.data() length:data.size()];

  UIImage* ui_image = [UIImage imageWithData:image_data scale:1];
  SkBitmap bitmap;
  if (ui_image) {
    bitmap = SkBitmap(
        skia::CGImageToSkBitmap(ui_image.CGImage, ui_image.size, false));
  }

  std::move(callback).Run(bitmap);
}

void SanitizedImageSource::DataDecoderDelegate::DecodeAnimation(
    const std::string& data,
    DecodeAnimationCallback callback) {
  NSData* image_data = [NSData dataWithBytes:data.data() length:data.size()];

  std::vector<data_decoder::mojom::AnimationFramePtr> decoded_images;
  std::vector<SkBitmap> frames =
      skia::ImageDataToSkBitmapsWithMaxSize(image_data, 1024);

  for (const SkBitmap& frame : frames) {
    auto image_frame = data_decoder::mojom::AnimationFrame::New();
    image_frame->bitmap = frame;
    image_frame->duration = base::Milliseconds(
        0.25);  // TODO: Figure out interpolation for the frames

    // ResizeImage(&image_frame->bitmap, shrink_to_fit, kMaxImageSizeInBytes);
    if (image_frame->bitmap.isNull()) {
      decoded_images.clear();
      break;
    }

    decoded_images.push_back(std::move(image_frame));
  }

  std::move(callback).Run(std::move(decoded_images));
}

SanitizedImageSource::SanitizedImageSource(ProfileIOS* profile)
    : SanitizedImageSource(profile,
                           profile->GetSharedURLLoaderFactory(),
                           std::make_unique<DataDecoderDelegate>()) {}

SanitizedImageSource::SanitizedImageSource(
    ProfileIOS* profile,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::unique_ptr<DataDecoderDelegate> delegate)
    : identity_manager_(IdentityManagerFactory::GetForProfile(profile)),
      url_loader_factory_(url_loader_factory),
      data_decoder_delegate_(std::move(delegate)) {}

SanitizedImageSource::~SanitizedImageSource() = default;

std::string SanitizedImageSource::GetSource() const {
  return chrome::kChromeUIImageHost;
}

void SanitizedImageSource::StartDataRequest(
    std::string_view path,
    web::URLDataSourceIOS::GotDataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  GURL url = GURL(chrome::kChromeUIImageURL).GetWithEmptyPath().Resolve(path);
  std::string_view image_url_or_params = url.query();
  if (url != GURL(base::StrCat(
                 {chrome::kChromeUIImageURL, "?", image_url_or_params}))) {
    std::move(callback).Run(nullptr);
    return;
  }

  RequestAttributes request_attributes;
  GURL image_url = GURL(image_url_or_params);
  bool send_auth_token = false;
  if (!image_url.is_valid()) {
    // Attempt to parse URL and additional options from params.
    auto params = ParseParams(image_url_or_params);

    auto url_it = params.find(kUrlKey);
    if (url_it == params.end()) {
      std::move(callback).Run(nullptr);
      return;
    }
    image_url = GURL(url_it->second);

    auto static_encode_it = params.find(kStaticEncodeKey);
    if (static_encode_it != params.end()) {
      request_attributes.static_encode = static_encode_it->second == "true";
    }

    auto encode_type_ir = params.find(kEncodeTypeKey);
    if (encode_type_ir != params.end()) {
      request_attributes.encode_type =
          encode_type_ir->second == "webp"
              ? RequestAttributes::EncodeType::kWebP
              : RequestAttributes::EncodeType::kPng;
    }

    auto google_photos_it = params.find(kIsGooglePhotosKey);
    if (google_photos_it != params.end() &&
        google_photos_it->second == "true" && IsGooglePhotosUrl(image_url)) {
      send_auth_token = true;
    }
  }

  if (image_url.SchemeIs(url::kHttpScheme)) {
    // Disallow any HTTP requests, treat them as a failure instead.
    std::move(callback).Run(nullptr);
    return;
  }

  request_attributes.image_url = image_url;

  // Download the image body.
  if (!send_auth_token) {
    StartImageDownload(std::move(request_attributes), std::move(callback));
    return;
  }

  // Request an auth token for downloading the image body.
  auto fetcher = std::make_unique<signin::PrimaryAccountAccessTokenFetcher>(
      "sanitized_image_source", identity_manager_,
      signin::ScopeSet({GaiaConstants::kPhotosModuleImageOAuth2Scope}),
      signin::PrimaryAccountAccessTokenFetcher::Mode::kImmediate,
      signin::ConsentLevel::kSignin);
  auto* fetcher_ptr = fetcher.get();
  fetcher_ptr->Start(base::BindOnce(
      [](const base::WeakPtr<SanitizedImageSource>& self,
         std::unique_ptr<signin::PrimaryAccountAccessTokenFetcher> fetcher,
         RequestAttributes request_attributes,
         web::URLDataSourceIOS::GotDataCallback callback,
         GoogleServiceAuthError error,
         signin::AccessTokenInfo access_token_info) {
        if (error.state() != GoogleServiceAuthError::NONE) {
          LOG(ERROR) << "Failed to authenticate for Google Photos in order to "
                        "download "
                     << request_attributes.image_url.spec()
                     << ". Error message: " << error.ToString();
          return;
        }

        request_attributes.access_token_info = access_token_info;

        if (self) {
          self->StartImageDownload(std::move(request_attributes),
                                   std::move(callback));
        }
      },
      weak_ptr_factory_.GetWeakPtr(), std::move(fetcher),
      std::move(request_attributes), std::move(callback)));
}

SanitizedImageSource::RequestAttributes::RequestAttributes() = default;
SanitizedImageSource::RequestAttributes::RequestAttributes(
    const RequestAttributes&) = default;
SanitizedImageSource::RequestAttributes::~RequestAttributes() = default;

void SanitizedImageSource::StartImageDownload(
    RequestAttributes request_attributes,
    web::URLDataSourceIOS::GotDataCallback callback) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("sanitized_image_source", R"(
        semantics {
          sender: "WebUI Sanitized Image Source"
          description:
            "This data source fetches an arbitrary image to be displayed in a "
            "WebUI."
          trigger:
            "When a WebUI triggers the download of chrome://image?<URL> or "
            "chrome://image?url=<URL>&isGooglePhotos=<bool> by e.g. setting "
            "that URL as a src on an img tag."
          data: "OAuth credentials for the user's Google Photos account when "
                "isGooglePhotos is true."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled by settings."
          policy_exception_justification:
            "This is a helper data source. It can be indirectly disabled by "
            "disabling the requester WebUI."
        })");
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = request_attributes.image_url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  if (request_attributes.access_token_info) {
    request->headers.SetHeader(
        net::HttpRequestHeaders::kAuthorization,
        "Bearer " + request_attributes.access_token_info->token);
  }

  auto loader =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&SanitizedImageSource::OnImageLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(loader),
                     std::move(request_attributes), std::move(callback)),
      network::SimpleURLLoader::kMaxBoundedStringDownloadSize);
}

std::string SanitizedImageSource::GetMimeType(std::string_view path) const {
  return "image/png";
}

bool SanitizedImageSource::ShouldReplaceExistingSource() const {
  // Leave the existing DataSource in place, otherwise we'll drop any pending
  // requests on the floor.
  return false;
}

void SanitizedImageSource::OnImageLoaded(
    std::unique_ptr<network::SimpleURLLoader> loader,
    RequestAttributes request_attributes,
    web::URLDataSourceIOS::GotDataCallback callback,
    std::unique_ptr<std::string> body) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Brave:
  if (pcdn_domain_.empty()) {
    pcdn_domain_ = brave_domains::GetServicesDomain("pcdn");
  }

  if (loader->NetError() == net::OK && body &&
      request_attributes.image_url.host() == pcdn_domain_ &&
      request_attributes.image_url.path().ends_with(".pad")) {
    std::string_view body_payload(body->data(), body->size());
    if (!brave::private_cdn::RemovePadding(&body_payload)) {
      std::move(callback).Run(nullptr);
      return;
    }

    *body = body_payload;
  }

  // Chromium:
  if (loader->NetError() != net::OK || !body) {
    std::move(callback).Run(nullptr);
    return;
  }

  if (request_attributes.static_encode) {
    data_decoder_delegate_->DecodeImage(
        *body,
        base::BindOnce(&SanitizedImageSource::EncodeAndReplyStaticImage,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(request_attributes), std::move(callback)));
    return;
  }

  data_decoder_delegate_->DecodeAnimation(
      *body,
      base::BindOnce(&SanitizedImageSource::OnAnimationDecoded,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(request_attributes), std::move(callback)));
}

void SanitizedImageSource::OnAnimationDecoded(
    RequestAttributes request_attributes,
    web::URLDataSourceIOS::GotDataCallback callback,
    std::vector<data_decoder::mojom::AnimationFramePtr> mojo_frames) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!mojo_frames.size()) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Re-encode as static image and send to requester.
  EncodeAndReplyStaticImage(std::move(request_attributes), std::move(callback),
                            mojo_frames[0]->bitmap);
}

void SanitizedImageSource::EncodeAndReplyStaticImage(
    RequestAttributes request_attributes,
    web::URLDataSourceIOS::GotDataCallback callback,
    const SkBitmap& bitmap) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](const SkBitmap& bitmap,
             RequestAttributes::EncodeType encode_type) {
            std::optional<std::vector<uint8_t>> result =
                encode_type == RequestAttributes::EncodeType::kWebP
                    ? gfx::WebpCodec::Encode(bitmap, /*quality=*/90)
                    : gfx::PNGCodec::EncodeBGRASkBitmap(
                          bitmap, /*discard_transparency=*/false);
            if (!result) {
              return base::MakeRefCounted<base::RefCountedBytes>();
            }
            return base::MakeRefCounted<base::RefCountedBytes>(
                std::move(result.value()));
          },
          bitmap, request_attributes.encode_type),
      std::move(callback));
  return;
}

void SanitizedImageSource::EncodeAndReplyAnimatedImage(
    web::URLDataSourceIOS::GotDataCallback callback,
    std::vector<data_decoder::mojom::AnimationFramePtr> mojo_frames) {
  std::vector<gfx::WebpCodec::Frame> frames;
  for (auto& mojo_frame : mojo_frames) {
    gfx::WebpCodec::Frame frame;
    frame.bitmap = mojo_frame->bitmap;
    frame.duration = mojo_frame->duration.InMilliseconds();
    frames.push_back(frame);
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](const std::vector<gfx::WebpCodec::Frame>& frames) {
            SkWebpEncoder::Options options;
            options.fCompression = SkWebpEncoder::Compression::kLossless;
            // Lower quality under kLosless compression means compress faster
            // into larger files.
            options.fQuality = 0;

            auto encoded = gfx::WebpCodec::EncodeAnimated(frames, options);
            if (encoded.has_value()) {
              return base::MakeRefCounted<base::RefCountedBytes>(
                  encoded.value());
            }

            return base::MakeRefCounted<base::RefCountedBytes>();
          },
          frames),
      std::move(callback));
}
