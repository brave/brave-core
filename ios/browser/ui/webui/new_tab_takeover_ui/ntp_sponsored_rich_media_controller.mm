/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/new_tab_takeover_ui/ntp_sponsored_rich_media_controller.h"

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
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_url_data_source_ios.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace {

  // This class is responsible for providing sponsored rich media content from the
// file system to the new tab page.

class NTPSponsoredRichMediaSourceIOS final : public BraveURLDataSourceIOS {
 public:
  // TODO(aseren): Add this function to WebUIIOSDataSource for rich media
  // background.
  // static NTPSponsoredRichMediaSourceIOS* CreateAndAdd(
  //     web::BrowserState* browser_state,
  //     ntp_background_images::NTPBackgroundImagesService* background_images_service);

  explicit NTPSponsoredRichMediaSourceIOS(
      ntp_background_images::NTPBackgroundImagesService* background_images_service);

  NTPSponsoredRichMediaSourceIOS(const NTPSponsoredRichMediaSourceIOS&) =
      delete;
  NTPSponsoredRichMediaSourceIOS& operator=(
      const NTPSponsoredRichMediaSourceIOS&) = delete;

  ~NTPSponsoredRichMediaSourceIOS() override;

  // BraveURLDataSourceIOS:
  std::string GetSource() const override;
  void StartDataRequest(const std::string& path,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool AllowCaching() const override;
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const override;

 private:
  void ReadFileCallback(GotDataCallback callback,
                        std::optional<std::string> input);

  void AllowAccess(const base::FilePath& file_path, GotDataCallback callback);
  void DenyAccess(GotDataCallback callback);

  const raw_ptr<ntp_background_images::NTPBackgroundImagesService>
      background_images_service_;  // Not owned.

  base::WeakPtrFactory<NTPSponsoredRichMediaSourceIOS> weak_factory_{this};
};

// TODO(aseren): Add this function to WebUIIOSDataSource for rich media
// background.
// // static
// NTPSponsoredRichMediaSourceIOS* NTPSponsoredRichMediaSourceIOS::CreateAndAdd(
//     web::BrowserState* browser_state,
//     ntp_background_images::NTPBackgroundImagesService* background_images_service) {
//   web::WebUIIOSDataSource::Add(
//       browser_state,
//       new NTPSponsoredRichMediaSourceIOS(background_images_service));
// }

NTPSponsoredRichMediaSourceIOS::NTPSponsoredRichMediaSourceIOS(
    ntp_background_images::NTPBackgroundImagesService* background_images_service)
    : background_images_service_(background_images_service) {}

NTPSponsoredRichMediaSourceIOS::~NTPSponsoredRichMediaSourceIOS() = default;

std::string NTPSponsoredRichMediaSourceIOS::GetSource() const {
  return kNewTabTakeoverHost;
  //return "brave://new-tab-takeover";
}

void NTPSponsoredRichMediaSourceIOS::StartDataRequest(
    const std::string& path,
    GotDataCallback callback) {
  //DCHECK(false);
  DCHECK(background_images_service_);
  if (!background_images_service_) {
    return DenyAccess(std::move(callback));
  }

  LOG(ERROR) << "FOOBAR.StartDataRequest " << path;

  const ntp_background_images::NTPSponsoredImagesData* const images_data =
      background_images_service_->GetSponsoredImagesData(
          /*super_referral=*/false,
          /*supports_rich_media=*/true);
  if (!images_data) {
    LOG(ERROR) << "FOOBAR.StartDataRequest images_data is null";
    return DenyAccess(std::move(callback));
  }

  const base::FilePath request_path = base::FilePath::FromUTF8Unsafe(path);
  const std::optional<base::FilePath> file_path =
      MaybeGetFilePathForRequestPath(request_path, images_data->campaigns);
  if (!file_path) {
    LOG(ERROR) << "FOOBAR.StartDataRequest file_path is null";
    return DenyAccess(std::move(callback));
  }

  AllowAccess(*file_path, std::move(callback));
}

std::string NTPSponsoredRichMediaSourceIOS::GetMimeType(
    const std::string& path) const {
  DCHECK(background_images_service_);
  std::string mime_type;
  const base::FilePath file_path = base::FilePath::FromUTF8Unsafe(path);
  if (!file_path.empty()) {
    LOG(ERROR) << "FOOBAR.GetMimeType file_path is " << file_path.value();
    net::GetWellKnownMimeTypeFromFile(file_path, &mime_type);
  } else {
    LOG(ERROR) << "FOOBAR.GetMimeType file_path is empty";
  }

  return mime_type;
}

bool NTPSponsoredRichMediaSourceIOS::AllowCaching() const {
  return false;
}

std::string NTPSponsoredRichMediaSourceIOS::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) const {
  DCHECK(background_images_service_);
  switch (directive) {
    // case network::mojom::CSPDirectiveName::FrameAncestors:
    //   return absl::StrFormat("frame-ancestors %s %s;", kBraveUINewTabURL,
    //                          kBraveUINewTabTakeoverURL);
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
      base::BindOnce(&ntp_background_images::ReadFileToString, file_path),
      base::BindOnce(&NTPSponsoredRichMediaSourceIOS::ReadFileCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPSponsoredRichMediaSourceIOS::DenyAccess(GotDataCallback callback) {
  std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
}

}  // namespace

NTPSponsoredRichMediaController::NTPSponsoredRichMediaController(
    web::WebUIIOS* web_ui, const GURL& url,
    ntp_background_images::NTPBackgroundImagesService* background_images_service)
    : web::WebUIIOSController(web_ui, url.host()) {
  // auto* source = new NTPSponsoredRichMediaSourceIOS(nullptr);
  // web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui), source);

  web::URLDataSourceIOS::Add(ProfileIOS::FromWebUIIOS(web_ui),
                             new NTPSponsoredRichMediaSourceIOS(background_images_service));

  // web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui),
  //                              new NTPSponsoredRichMediaSourceIOS(
  //                                  background_images_service_));
}

// NTPSponsoredRichMediaController::NTPSponsoredRichMediaController(
//     web::WebUIIOS* web_ui, const GURL& url)
//     : web::WebUIIOSController(web_ui, url.host()) {
//   // auto* source = new NTPSponsoredRichMediaSourceIOS(nullptr);
//   // web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui), source);

//   web::URLDataSourceIOS::Add(ProfileIOS::FromWebUIIOS(web_ui),
//                              new NTPSponsoredRichMediaSourceIOS(nullptr));

//   // web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui),
//   //                              new NTPSponsoredRichMediaSourceIOS(
//   //                                  background_images_service_));
// }

NTPSponsoredRichMediaController::~NTPSponsoredRichMediaController() = default;
