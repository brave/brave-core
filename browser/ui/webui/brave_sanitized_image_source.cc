// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_sanitized_image_source.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/url_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/blink/public/common/loader/network_utils.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "url/url_util.h"

namespace {
constexpr char kUrlKey[] = "url";
constexpr char kTargetSizeKey[] = "target_size";
constexpr char kChromeUIBraveImageURL[] = "chrome://brave-image/";
constexpr char kChromeUIBraveImageHost[] = "brave-image";

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

scoped_refptr<base::RefCountedMemory> EncodeImage(
    SkBitmap bitmap,
    const gfx::Size& target_size) {
  if (!target_size.IsEmpty() && bitmap.width() > 0 && bitmap.height() > 0) {
    // Resize to the target size, keeping the aspect ratio:
    auto target_width = target_size.width();
    auto target_height = target_size.height();
    auto width_with_ratio = target_height * bitmap.width() / bitmap.height();
    if (width_with_ratio > target_width) {
      target_width = width_with_ratio;
    } else {
      target_height = target_width * bitmap.height() / bitmap.width();
    }

    // Only resize if the target size is smaller than the original size:
    if (target_width < bitmap.width() && target_height < bitmap.height()) {
      bitmap = skia::ImageOperations::Resize(bitmap,
                                             skia::ImageOperations::RESIZE_BEST,
                                             target_width, target_height);
    }
  }

  std::optional<std::vector<uint8_t>> result =
      gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap,
                                            /*discard_transparency=*/false);
  if (!result) {
    return nullptr;
  }
  return base::MakeRefCounted<base::RefCountedBytes>(std::move(result.value()));
}

}  // namespace

BraveSanitizedImageSource::~BraveSanitizedImageSource() = default;

void BraveSanitizedImageSource::OnImageLoaded(
    std::unique_ptr<network::SimpleURLLoader> loader,
    RequestAttributes request_attributes,
    content::URLDataSource::GotDataCallback callback,
    std::unique_ptr<std::string> body) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (loader->NetError() != net::OK || !body) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Lazily initialize the pcdn domain
  if (pcdn_domain_.empty()) {
    pcdn_domain_ = brave_domains::GetServicesDomain("pcdn");
  }

  if (loader->NetError() == net::OK && body &&
      request_attributes.image_url.host_piece() == pcdn_domain_ &&
      request_attributes.image_url.path_piece().ends_with(".pad")) {
    std::string_view body_payload(body->data(), body->size());
    if (!brave::private_cdn::RemovePadding(&body_payload)) {
      std::move(callback).Run(nullptr);
      return;
    }

    *body = body_payload;
  }

  base::span<const uint8_t> bytes = base::as_byte_span(*body);

  auto cb =
      base::BindOnce(&BraveSanitizedImageSource::EncodeAndReplyStaticImage,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(request_attributes), std::move(callback));

  data_decoder::DecodeImage(
      &data_decoder_, bytes, data_decoder::mojom::ImageCodec::kDefault,
      /*shrink_to_fit=*/true, data_decoder::kDefaultMaxSizeInBytes,
      /*desired_image_frame_size=*/gfx::Size(), std::move(cb));
}

void BraveSanitizedImageSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::string_view image_url_or_params = url.query();

  if (url !=
      GURL(base::StrCat({kChromeUIBraveImageURL, "?", image_url_or_params}))) {
    std::move(callback).Run(nullptr);
    return;
  }

  RequestAttributes request_attributes;
  GURL image_url = GURL(image_url_or_params);
  if (!image_url.is_valid()) {
    // Attempt to parse URL and additional options from params.
    auto params = ParseParams(image_url_or_params);

    auto url_it = params.find(kUrlKey);
    if (url_it == params.end()) {
      std::move(callback).Run(nullptr);
      return;
    }
    image_url = GURL(url_it->second);

    auto target_size_it = params.find(kTargetSizeKey);

    if (target_size_it != params.end()) {
      // Parse target_size as "WxH", i.e. "100x200".
      const auto& target_str = std::string_view(target_size_it->second);
      const auto values = base::SplitStringPiece(
          target_str, "x", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      if (values.size() == 2) {
        unsigned int width, height;
        if (base::StringToUint(values[0], &width) &&
            base::StringToUint(values[1], &height)) {
          request_attributes.target_size = gfx::Size(width, height);
        }
      }
    }
  }

  if (image_url.SchemeIs(url::kHttpScheme)) {
    // Disallow any HTTP requests, treat them as a failure instead.
    std::move(callback).Run(nullptr);
    return;
  }

  request_attributes.image_url = image_url;

  StartImageDownload(std::move(request_attributes), std::move(callback));
}

BraveSanitizedImageSource::BraveSanitizedImageSource(Profile* profile)
    : BraveSanitizedImageSource(profile,
                                profile->GetDefaultStoragePartition()
                                    ->GetURLLoaderFactoryForBrowserProcess()) {}

BraveSanitizedImageSource::BraveSanitizedImageSource(
    Profile* profile,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool BraveSanitizedImageSource::AllowCaching() {
  return false;
}
std::string BraveSanitizedImageSource::GetSource() {
  return kChromeUIBraveImageHost;
}

void BraveSanitizedImageSource::EncodeAndReplyStaticImage(
    RequestAttributes request_attributes,
    content::URLDataSource::GotDataCallback callback,
    const SkBitmap& bitmap) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&EncodeImage, std::move(bitmap),
                     request_attributes.target_size),
      std::move(callback));
  return;
}

void BraveSanitizedImageSource::StartImageDownload(
    RequestAttributes request_attributes,
    content::URLDataSource::GotDataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("sanitized_image_source", R"(
      semantics {
        sender: "Brave WebUI Sanitized Image Source"
        description:
          "This data source fetches an arbitrary image to be displayed in a "
          "Brave WebUI."
        trigger:
          "When a WebUI triggers the download of chrome://brave-image?<URL> or "
          "chrome://brave-image?url=<URL> by e.g. setting "
          "that URL as a src on an img tag."
        data: "No user data is included."
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
  request->headers.SetHeader("Accept",
                             blink::network_utils::ImageAcceptHeader());

  auto loader =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveSanitizedImageSource::OnImageLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(loader),
                     std::move(request_attributes), std::move(callback)),
      network::SimpleURLLoader::kMaxBoundedStringDownloadSize);
}

std::string BraveSanitizedImageSource::GetMimeType(const GURL&) {
  return "image/png";
}
