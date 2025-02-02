/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_source.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/thread_pool.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

std::optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::optional<std::string>();
  }
  return contents;
}

}  // namespace

NTPSponsoredRichMediaSource::NTPSponsoredRichMediaSource(
    NTPBackgroundImagesService* service)
    : service_(service), weak_factory_(this) {}

NTPSponsoredRichMediaSource::~NTPSponsoredRichMediaSource() = default;

std::string NTPSponsoredRichMediaSource::GetSource() {
  return kNTPSponsoredRichMediaUrl;
}

void NTPSponsoredRichMediaSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string request_path = URLDataSource::URLToRequestPath(url);
  const std::optional<base::FilePath> file_path =
      MaybeGetFilePathForRequestPath(request_path);
  if (!file_path) {
    // Deny access.
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  scoped_refptr<base::RefCountedMemory>()));

    return;
  }

  // Allow access.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, *file_path),
      base::BindOnce(&NTPSponsoredRichMediaSource::ReadFileCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPSponsoredRichMediaSource::ReadFileCallback(
    GotDataCallback callback,
    std::optional<std::string> input) {
  if (!input) {
    return std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string NTPSponsoredRichMediaSource::GetMimeType(const GURL& url) {
  std::string mime_type;
  const base::FilePath::StringType file_path_extension =
      base::FilePath::FromUTF8Unsafe(url.path_piece()).Extension();
  if (!file_path_extension.empty()) {
    net::GetWellKnownMimeTypeFromExtension(file_path_extension.substr(1),
                                           &mime_type);
  }

  return mime_type;
}

bool NTPSponsoredRichMediaSource::AllowCaching() {
  return false;
}

std::string NTPSponsoredRichMediaSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) {
  switch (directive) {
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return std::string("frame-ancestors ") + kBraveUINewTabURL + ";";
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
      // from content::URLDataSource::GetContentSecurityPolicy().
      return "";
  }
}

std::optional<base::FilePath>
NTPSponsoredRichMediaSource::MaybeGetFilePathForRequestPath(
    const std::string& request_path) {
  const NTPSponsoredImagesData* const images_data =
      service_->GetBrandedImagesData(false);
  if (!images_data) {
    // No sponsored images data available, deny access.
    return std::nullopt;
  }

  const std::string request_dir_name =
      base::FilePath::FromUTF8Unsafe(request_path).DirName().AsUTF8Unsafe();
  const std::string request_base_name =
      base::FilePath::FromUTF8Unsafe(request_path).BaseName().AsUTF8Unsafe();

  for (const auto& campaign : images_data->campaigns) {
    for (const auto& creative : campaign.backgrounds) {
      // Sandbox the request to the creative directory to prevent path
      // traversal, denying access to other directories.
      const base::FilePath creative_file_path = creative.file_path.DirName();
      const std::string creative_base_name =
          creative_file_path.BaseName().AsUTF8Unsafe();
      if (creative_base_name == request_dir_name) {
        return creative_file_path.Append(request_base_name);
      }
    }
  }

  // Path traversal, deny access.
  return std::nullopt;
}

}  // namespace ntp_background_images
