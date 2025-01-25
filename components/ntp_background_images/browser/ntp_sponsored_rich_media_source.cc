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
  return kRichMediaURL;
}

void NTPSponsoredRichMediaSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  VLOG(6) << "Start data request for Rich Media asset at " << url;

  const std::string path = URLDataSource::URLToRequestPath(url);
  std::optional<base::FilePath> file_path = GetLocalFilePathFor(path);
  // DONE. TODO(tmancey): @aseren would it make sense to return std::optional,
  // but I am unsure how this could fail in the first place.
  if (!file_path) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  scoped_refptr<base::RefCountedMemory>()));

    return;
  }

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
    // DONE. TODO(tmancey): @aseren is it intentional that we do not call the
    // callback if this function fails? Surely we need to?
    return std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string NTPSponsoredRichMediaSource::GetMimeType(const GURL& url) {
  std::string mime_type;
  std::string file_path_ext = base::FilePath(url.path_piece()).Extension();
  if (!file_path_ext.empty()) {
    net::GetWellKnownMimeTypeFromExtension(file_path_ext.substr(1), &mime_type);
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
      return std::string("frame-ancestors ") + kBraveNewTabPageURL + ";";
    case network::mojom::CSPDirectiveName::Sandbox:
      return std::string("sandbox allow-scripts;");
    case network::mojom::CSPDirectiveName::DefaultSrc:
      return std::string("default-src 'none';");
    case network::mojom::CSPDirectiveName::BaseURI:
      return std::string("base-uri 'none';");
    case network::mojom::CSPDirectiveName::FormAction:
      return std::string("form-action 'none';");
    case network::mojom::CSPDirectiveName::ScriptSrc:
      return std::string("script-src 'self' 'unsafe-inline';");
    case network::mojom::CSPDirectiveName::StyleSrc:
      return std::string("style-src 'self';");
    case network::mojom::CSPDirectiveName::ImgSrc:
      return std::string("img-src 'self';");
    case network::mojom::CSPDirectiveName::MediaSrc:
      return std::string("media-src 'self';");
    default:
      return content::URLDataSource::GetContentSecurityPolicy(directive);
  }
}

// DONE. TODO(tmancey): @aseren do we need all these nested loops after our
// discussion? And are we sure NOTREACHED() is safe? GetLocalFilePathFor is
// confusing, maybe me :-) because it is not clear what it is doing.
std::optional<base::FilePath> NTPSponsoredRichMediaSource::GetLocalFilePathFor(
    const std::string& path) {
  VLOG(6) << "Get local path for Rich Media asset at " << path;

  auto* images_data = service_->GetBrandedImagesData(false);
  if (!images_data) {
    return std::nullopt;
  }

  const auto basename_from_path =
      base::FilePath::FromUTF8Unsafe(path).BaseName();
  const auto dirname_from_path =
      base::FilePath::FromUTF8Unsafe(path).DirName().AsUTF8Unsafe();

  for (const auto& campaign : images_data->campaigns) {
    for (const auto& background : campaign.backgrounds) {
      if (dirname_from_path == background.creative_instance_id) {
        const auto assets_directory_path = background.wallpaper_file.DirName();
        const auto local_file_path =
            assets_directory_path.Append(basename_from_path);

        VLOG(6) << "Found matching campaign for Rich Media asset at "
                << local_file_path.AsUTF8Unsafe();
        return local_file_path;
      }
    }
  }

  return std::nullopt;
}

}  // namespace ntp_background_images
