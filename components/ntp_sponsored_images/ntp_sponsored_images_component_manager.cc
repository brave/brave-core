/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_component_manager.h"

#include <algorithm>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_data.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_internal_data.h"
#include "brave/components/ntp_sponsored_images/regional_component_data.h"
#include "brave/components/ntp_sponsored_images/switches.h"
#include "brave/vendor/bat-native-ads/src/bat/ads/internal/locale_helper.h"
#include "content/public/browser/browser_context.h"

namespace {

constexpr char kPhotoJsonFilename[] = "photo.json";
constexpr char kComponentName[] = "NTP sponsored images";

constexpr char kLogoImageURLPath[] = "logo.imageUrl";
constexpr char kLogoAltPath[] = "logo.alt";
constexpr char kLogoCompanyNamePath[] = "logo.companyName";
constexpr char kLogoDestinationURLPath[] = "logo.destinationUrl";
constexpr char kWallpapersPath[] = "wallpapers";
constexpr char kWallpaperImageURLPath[] = "imageUrl";

std::string ReadPhotoJsonData(const base::FilePath& photo_json_file_path) {
  std::string contents;
  bool success = base::ReadFileToString(photo_json_file_path, &contents);
  if (!success || contents.empty()) {
    DVLOG(2) << "ReadPhotoJsonData: cannot "
             << "read photo.json file " << photo_json_file_path;
  }
  return contents;
}

}  // namespace

NTPSponsoredImagesComponentManager::NTPSponsoredImagesComponentManager(
    BraveComponent::Delegate* delegate,
    component_updater::ComponentUpdateService* cus)
    : BraveComponent(delegate),
      weak_factory_(this) {
  // Early return for test.
  if (!cus)
    return;

  // Flag override for testing or demo purposes
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kNTPBrandedDataPathForTesting));
  if (!forced_local_path.empty()) {
    LOG(ERROR)
        << "NTP Sponsored Image package will be loaded from local path at: "
        << forced_local_path.LossyDisplayName();
    OnComponentReady("", forced_local_path, "");
    return;
  }

  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  if (const auto& data = GetRegionalComponentData(
          helper::Locale::GetRegionCode(locale))) {
    Register(kComponentName,
             data->component_id,
             data->component_base64_public_key);
    cus_ = cus;
    cus_->AddObserver(this);
  }
}

NTPSponsoredImagesComponentManager::~NTPSponsoredImagesComponentManager() {
  if (cus_)
    cus_->RemoveObserver(this);
}

void NTPSponsoredImagesComponentManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void NTPSponsoredImagesComponentManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void NTPSponsoredImagesComponentManager::AddDataSource(
    content::BrowserContext* browser_context) {
  if (!internal_images_data_)
    return;

  if (!internal_images_data_->logo_image_file.empty()) {
    content::URLDataSource::Add(
        browser_context,
        std::make_unique<NTPSponsoredImageSource>(*internal_images_data_));
  }
}

base::Optional<NTPSponsoredImagesData>
NTPSponsoredImagesComponentManager::GetLatestSponsoredImagesData() const {
  if (internal_images_data_)
    return NTPSponsoredImagesData(*internal_images_data_);

  return base::nullopt;
}

void NTPSponsoredImagesComponentManager::ReadPhotoJsonFileAndNotify() {
  // Reset previous data.
  internal_images_data_.reset();

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadPhotoJsonData, photo_json_file_path_),
      base::BindOnce(&NTPSponsoredImagesComponentManager::OnGetPhotoJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPSponsoredImagesComponentManager::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& installed_dir,
    const std::string& manifest) {
  photo_json_file_path_ = installed_dir.AppendASCII(kPhotoJsonFilename);
  ReadPhotoJsonFileAndNotify();
}

void NTPSponsoredImagesComponentManager::OnEvent(Events event,
                                                 const std::string& id) {
  if (!id.empty() &&
      id == component_id() &&
      event == Events::COMPONENT_UPDATED) {
      ReadPhotoJsonFileAndNotify();
  }
}

void NTPSponsoredImagesComponentManager::OnGetPhotoJsonData(
    const std::string& photo_json) {
  ParseAndCachePhotoJsonData(photo_json);
  NotifyObservers();
}

void NTPSponsoredImagesComponentManager::ParseAndCachePhotoJsonData(
    const std::string& photo_json) {
  base::Optional<base::Value> photo_value = base::JSONReader::Read(photo_json);
  if (photo_value) {
    internal_images_data_.reset(new NTPSponsoredImagesInternalData);

    // Resources are stored with json file in the same directory.
    base::FilePath base_dir = photo_json_file_path_.DirName();

    if (auto* logo_image_url = photo_value->FindStringPath(kLogoImageURLPath)) {
      internal_images_data_->logo_image_file =
          base_dir.AppendASCII(*logo_image_url);
    }

    if (auto* logo_alt_text = photo_value->FindStringPath(kLogoAltPath)) {
      internal_images_data_->logo_alt_text = *logo_alt_text;
    }

    if (auto* logo_company_name =
            photo_value->FindStringPath(kLogoCompanyNamePath)) {
      internal_images_data_->logo_company_name = *logo_company_name;
    }

    if (auto* logo_destination_url =
            photo_value->FindStringPath(kLogoDestinationURLPath)) {
      internal_images_data_->logo_destination_url = *logo_destination_url;
    }

    if (auto* wallpaper_image_urls =
            photo_value->FindListPath(kWallpapersPath)) {
      for (const auto& value : wallpaper_image_urls->GetList()) {
        internal_images_data_->wallpaper_image_files.push_back(
            base_dir.AppendASCII(
                *value.FindStringPath(kWallpaperImageURLPath)));
      }
    }
  }
}

void NTPSponsoredImagesComponentManager::NotifyObservers() {
  for (auto& observer : observer_list_) {
    if (internal_images_data_)
      observer.OnUpdated(NTPSponsoredImagesData(*internal_images_data_));
    else
      observer.OnUpdated(NTPSponsoredImagesData());
  }
}

void NTPSponsoredImagesComponentManager::ResetInternalImagesDataForTest() {
  internal_images_data_.reset();
}
