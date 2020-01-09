/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/ntp_sponsored_image_source.h"

#include "base/strings/stringprintf.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_component_manager.h"
#include "brave/components/ntp_sponsored_images/url_constants.h"
#include "content/public/browser/browser_thread.h"

namespace {
constexpr char kInvalidSource[] = "";
}  // namespace

NTPSponsoredImageSource::NTPSponsoredImageSource(
    base::WeakPtr<NTPSponsoredImagesComponentManager> manager,
    const std::string& image_file_path,
    size_t wallpaper_index,
    Type type)
    : manager_(manager),
      image_file_path_(image_file_path),
      wallpaper_index_(wallpaper_index),
      type_(type) {
}

NTPSponsoredImageSource::~NTPSponsoredImageSource() = default;

std::string NTPSponsoredImageSource::GetSource() {
  if (!manager_ || !manager_->IsValidImage(type_, wallpaper_index_))
    return kInvalidSource;

  return IsLogoType() ? kBrandedLogoPath : GetWallpaperPath();
}

void NTPSponsoredImageSource::StartDataRequest(
    const std::string& path,
    const content::WebContents::Getter& wc_getter,
    const GotDataCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(simonhong): Read |image_file_path| and run |callback|.
  NOTIMPLEMENTED();
}

std::string NTPSponsoredImageSource::GetMimeType(const std::string& path) {
  return IsLogoType() ? "image/png" : "image/jpg";
}

bool NTPSponsoredImageSource::IsLogoType() const {
  return type_ == Type::TYPE_LOGO;
}

std::string NTPSponsoredImageSource::GetWallpaperPath() const {
  DCHECK_EQ(type_, Type::TYPE_LOGO);
  // Assemble path like branded-wallpaper-2.jpg
  return base::StringPrintf("%s-%zu.jpg",
                            kBrandedWallpaperPathPrefix, wallpaper_index_);
}
