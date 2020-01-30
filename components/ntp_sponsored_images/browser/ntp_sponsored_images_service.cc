/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/optional.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_component_installer.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_image_source.h"
#include "brave/components/ntp_sponsored_images/browser/regional_component_data.h"
#include "brave/components/ntp_sponsored_images/browser/switches.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/url_data_source.h"

namespace ntp_sponsored_images {

namespace {

constexpr char kPhotoJsonFilename[] = "photo.json";

constexpr char kLogoImageURLPath[] = "logo.imageUrl";
constexpr char kLogoAltPath[] = "logo.alt";
constexpr char kLogoCompanyNamePath[] = "logo.companyName";
constexpr char kLogoDestinationURLPath[] = "logo.destinationUrl";
constexpr char kWallpapersPath[] = "wallpapers";
constexpr char kWallpaperImageURLPath[] = "imageUrl";
constexpr char kSchemaVersionPath[] = "schemaVersion";

constexpr int kExpectedSchemaVersion = 1;

std::string ReadPhotosManifest(const base::FilePath& photos_manifest_path) {
  std::string contents;
  bool success = base::ReadFileToString(photos_manifest_path, &contents);
  if (!success || contents.empty()) {
    DVLOG(2) << "ReadPhotosManifest: cannot "
             << "read photo.json file " << photos_manifest_path;
  }
  return contents;
}

}  // namespace

NTPSponsoredImagesService::NTPSponsoredImagesService(
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
    RegisterNTPSponsoredImagesComponent(cus, data.value(),
        base::BindRepeating(&NTPSponsoredImagesService::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

NTPSponsoredImagesService::~NTPSponsoredImagesService() {}

void NTPSponsoredImagesService::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void NTPSponsoredImagesService::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool NTPSponsoredImagesService::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

void NTPSponsoredImagesService::AddDataSource(
    content::BrowserContext* browser_context) {
  content::URLDataSource::Add(browser_context,
                              std::make_unique<NTPSponsoredImageSource>(this));
}

NTPSponsoredImagesData*
NTPSponsoredImagesService::GetSponsoredImagesData() const {
  return images_data_.get();
}

void NTPSponsoredImagesService::OnComponentReady(
    const base::FilePath& installed_dir) {
  // image list is no longer valid after the component has been updated
  images_data_.reset();
  NotifyObservers();

  photos_manifest_path_ = installed_dir.AppendASCII(kPhotoJsonFilename);
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadPhotosManifest, photos_manifest_path_),
      base::BindOnce(&NTPSponsoredImagesService::OnGetPhotoJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPSponsoredImagesService::OnGetPhotoJsonData(
    const std::string& photo_json) {
  base::Optional<base::Value> photo_value = base::JSONReader::Read(photo_json);
  if (photo_value) {
    // Resources are stored with json file in the same directory.
    base::FilePath base_dir = photos_manifest_path_.DirName();

    base::Optional<int> incomingSchemaVersion =
        photo_value->FindIntPath(kSchemaVersionPath);
    const bool schemaVersionIsValid = incomingSchemaVersion &&
        *incomingSchemaVersion == kExpectedSchemaVersion;
    if (!schemaVersionIsValid) {
      LOG(ERROR) <<
      "Incoming NTP Sponsored images component data was not valid."
      "Schema version was " <<
      (incomingSchemaVersion
          ? std::to_string(*incomingSchemaVersion)
          : "missing") <<
      ", but we expected " << kExpectedSchemaVersion;
      images_data_.reset(nullptr);
      NotifyObservers();
      return;
    }

    images_data_.reset(new NTPSponsoredImagesData);

    if (auto* logo_image_url = photo_value->FindStringPath(kLogoImageURLPath)) {
      images_data_->logo_image_file =
          base_dir.AppendASCII(*logo_image_url);
    }

    if (auto* logo_alt_text = photo_value->FindStringPath(kLogoAltPath)) {
      images_data_->logo_alt_text = *logo_alt_text;
    }

    if (auto* logo_company_name =
            photo_value->FindStringPath(kLogoCompanyNamePath)) {
      images_data_->logo_company_name = *logo_company_name;
    }

    if (auto* logo_destination_url =
            photo_value->FindStringPath(kLogoDestinationURLPath)) {
      images_data_->logo_destination_url = *logo_destination_url;
    }

    if (auto* wallpaper_image_urls =
            photo_value->FindListPath(kWallpapersPath)) {
      for (const auto& value : wallpaper_image_urls->GetList()) {
        images_data_->wallpaper_image_files.push_back(
            base_dir.AppendASCII(
                *value.FindStringPath(kWallpaperImageURLPath)));
      }
    }
    NotifyObservers();
  }
}

void NTPSponsoredImagesService::NotifyObservers() {
  for (auto& observer : observer_list_) {
    observer.OnUpdated(images_data_.get());
  }
}

void NTPSponsoredImagesService::ResetImagesDataForTest() {
  images_data_.reset();
}

}  // namespace ntp_sponsored_images
