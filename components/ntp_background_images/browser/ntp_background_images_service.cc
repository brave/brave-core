/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <algorithm>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"
#include "brave/components/ntp_background_images/browser/regional_component_data.h"
#include "brave/components/ntp_background_images/browser/switches.h"

namespace ntp_background_images {

namespace {

constexpr char kPhotoJsonFilename[] = "photo.json";

std::string ReadPhotosManifest(const base::FilePath& photos_manifest_path) {
  std::string contents;
  bool success = base::ReadFileToString(photos_manifest_path, &contents);
  if (!success || contents.empty()) {
    DVLOG(2) << "ReadPhotosManifest: cannot "
             << "read photo.json file " << photos_manifest_path;
  }
  return contents;
}

NTPBackgroundImagesData* GetDemoWallpaper() {
  static auto demo = std::make_unique<NTPBackgroundImagesData>();
  demo->url_prefix = "chrome://newtab/ntp-dummy-brandedwallpaper/";
  demo->backgrounds = {
      { base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")), { 3988, 2049 } },
      { base::FilePath(FILE_PATH_LITERAL("wallpaper2.jpg")), { 5233, 3464 } },
      { base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")), {  0, 0 } },
  };
  demo->logo_alt_text = "Technikke: For music lovers.";
  demo->logo_company_name = "Technikke";
  demo->logo_destination_url = "https://brave.com";
  return demo.get();
}

}  // namespace

NTPBackgroundImagesService::NTPBackgroundImagesService(
    component_updater::ComponentUpdateService* cus)
    : weak_factory_(this) {
  // Flag override for testing or demo purposes
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kNTPBrandedDataPathForTesting));
  if (!forced_local_path.empty()) {
    LOG(ERROR)
        << "NTP Sponsored Image package will be loaded from local path at: "
        << forced_local_path.LossyDisplayName();
    OnComponentReady(forced_local_path);
    return;
  }

  // Early return for test.
  if (!cus)
    return;

  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  if (const auto& data = GetRegionalComponentData(
          brave_ads::LocaleHelper::GetCountryCode(locale))) {
    RegisterNTPBackgroundImagesComponent(cus, data.value(),
        base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

NTPBackgroundImagesService::~NTPBackgroundImagesService() {}

void NTPBackgroundImagesService::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void NTPBackgroundImagesService::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool NTPBackgroundImagesService::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

NTPBackgroundImagesData*
NTPBackgroundImagesService::GetBackgroundImagesData() const {
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo))
    return GetDemoWallpaper();

  if (images_data_ && images_data_->IsValid())
    return images_data_.get();

  return nullptr;
}

void NTPBackgroundImagesService::OnComponentReady(
    const base::FilePath& installed_dir) {
  // image list is no longer valid after the component has been updated
  images_data_.reset();
  installed_dir_ = installed_dir;
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadPhotosManifest,
                     installed_dir.AppendASCII(kPhotoJsonFilename)),
      base::BindOnce(&NTPBackgroundImagesService::OnGetPhotoJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnGetPhotoJsonData(
    const std::string& photo_json) {
  images_data_.reset(new NTPBackgroundImagesData(photo_json, installed_dir_));
  NotifyObservers();
}

void NTPBackgroundImagesService::NotifyObservers() {
  for (auto& observer : observer_list_) {
    observer.OnUpdated(images_data_.get());
  }
}

}  // namespace ntp_background_images
