/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ntp_background_images/ntp_sponsored_rich_media_source_ios.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/thread_pool.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_util.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "net/base/mime_util.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ntp_background_images {

// TODO(aseren): Add this function to WebUIIOSDataSource for rich media
// background.
// // static
// NTPSponsoredRichMediaSourceIOS* NTPSponsoredRichMediaSourceIOS::CreateAndAdd(
//     web::BrowserState* browser_state,
//     NTPBackgroundImagesService* background_images_service) {
//   web::WebUIIOSDataSource::Add(
//       browser_state,
//       new NTPSponsoredRichMediaSourceIOS(background_images_service));
// }

NTPSponsoredRichMediaSourceIOS::NTPSponsoredRichMediaSourceIOS(
    NTPBackgroundImagesService* background_images_service)
    : background_images_service_(background_images_service) {}

NTPSponsoredRichMediaSourceIOS::~NTPSponsoredRichMediaSourceIOS() = default;

std::string NTPSponsoredRichMediaSourceIOS::GetSource() const {
  return kNTPNewTabTakeoverRichMediaUrl;
}

void NTPSponsoredRichMediaSourceIOS::StartDataRequest(
    const std::string& path,
    GotDataCallback callback) {
  if (!background_images_service_) {
    return DenyAccess(std::move(callback));
  }

  const NTPSponsoredImagesData* const images_data =
      background_images_service_->GetSponsoredImagesData(
          /*super_referral=*/false,
          /*supports_rich_media=*/true);
  if (!images_data) {
    return DenyAccess(std::move(callback));
  }

  const base::FilePath request_path = base::FilePath::FromUTF8Unsafe(path);
  const std::optional<base::FilePath> file_path =
      MaybeGetFilePathForRequestPath(request_path, images_data->campaigns);
  if (!file_path) {
    return DenyAccess(std::move(callback));
  }

  AllowAccess(*file_path, std::move(callback));
}

std::string NTPSponsoredRichMediaSourceIOS::GetMimeType(
    const std::string& path) const {
  std::string mime_type;
  const base::FilePath file_path = base::FilePath::FromUTF8Unsafe(path);
  if (!file_path.empty()) {
    net::GetWellKnownMimeTypeFromFile(file_path, &mime_type);
  }

  return mime_type;
}

bool NTPSponsoredRichMediaSourceIOS::AllowCaching() const {
  return false;
}

std::string NTPSponsoredRichMediaSourceIOS::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) const {
  switch (directive) {
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return absl::StrFormat("frame-ancestors %s %s;", kBraveUINewTabURL,
                             kBraveUINewTabTakeoverURL);
    case network::mojom::CSPDirectiveName::Sandbox:
      return "sandbox allow-scripts;";
    case network::mojom::CSPDirectiveName::DefaultSrc:
      return "default-src 'none';";
    case network::mojom::CSPDirectiveName::BaseURI:
      return "base-uri 'none';";
    case network::mojom::CSPDirectiveName::FormAction:
      return "form-action 'none';";
    case network::mojom::CSPDirectiveName::ScriptSrc:
      return "script-src 'self';";
    case network::mojom::CSPDirectiveName::StyleSrc:
      return "style-src 'self';";
    case network::mojom::CSPDirectiveName::FontSrc:
      return "font-src 'self';";
    case network::mojom::CSPDirectiveName::ImgSrc:
      return "img-src 'self';";
    case network::mojom::CSPDirectiveName::MediaSrc:
      return "media-src 'self';";
    case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
      return "require-trusted-types-for 'script';";
    case network::mojom::CSPDirectiveName::TrustedTypes:
      return "trusted-types;";
    default:
      // Return empty CSP to avoid inheriting potentially permissive defaults
      // from `content::URLDataSource::GetContentSecurityPolicy()`.
      return std::string();
  }
}

void NTPSponsoredRichMediaSourceIOS::ReadFileCallback(
    GotDataCallback callback,
    std::optional<std::string> input) {
  if (!input) {
    return std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

void NTPSponsoredRichMediaSourceIOS::AllowAccess(
    const base::FilePath& file_path,
    GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, file_path),
      base::BindOnce(&NTPSponsoredRichMediaSourceIOS::ReadFileCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPSponsoredRichMediaSourceIOS::DenyAccess(GotDataCallback callback) {
  std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
}

}  // namespace ntp_background_images
