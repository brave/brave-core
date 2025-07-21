/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_update_util.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_images_component_data.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/service/variations_service_utils.h"

namespace ntp_background_images {

namespace {

constexpr char kNTPManifestFile[] = "photo.json";
constexpr char kNTPSponsoredManifestFile[] = "campaigns.json";

// If registered component is for sponsored content, it has
// `kNTPSponsoredManifestFile` in |installed_dir|. Otherwise, it has
// `kNTPManifestFile` for super referral.
std::string HandleComponentData(const base::FilePath& installed_dir,
                                const std::string& manifest_file) {
  const base::FilePath file_path = installed_dir.AppendASCII(manifest_file);

  std::string contents;
  const bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(6) << "Cannot read NTP component " << manifest_file
            << " manifest file";
  }

  return contents;
}

// The variations service derives the country code from the client's IP address.
std::string GetVariationsCountryCode(
    variations::VariationsService* variations_service) {
  std::string country_code;

  if (variations_service) {
    country_code = variations_service->GetLatestCountry();
  }

  if (country_code.empty()) {
    // May be empty on first run after a fresh install, so fall back to the
    // permanently stored variations or device country code on first run.
    country_code = variations::GetCurrentCountryCode(variations_service);
  }

  // Convert the country code to an ISO 3166-1 alpha-2 format. This ensures the
  // country code is in uppercase, as required by the standard.
  return base::ToUpperASCII(country_code);
}

}  // namespace

// static
void NTPBackgroundImagesService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  registry->RegisterStringPref(
      prefs::kNewTabPageCachedSuperReferralComponentData, std::string());
  registry->RegisterStringPref(
      prefs::kNewTabPageCachedSuperReferralCode, std::string());
  registry->RegisterBooleanPref(
      prefs::kNewTabPageGetInitialSuperReferralComponentInProgress, false);
}

NTPBackgroundImagesService::NTPBackgroundImagesService(
    variations::VariationsService* variations_service,
    component_updater::ComponentUpdateService* component_update_service,
    PrefService* pref_service)
    : variations_service_(variations_service),
      component_update_service_(component_update_service),
      pref_service_(pref_service) {}

NTPBackgroundImagesService::~NTPBackgroundImagesService() = default;

void NTPBackgroundImagesService::Init() {
  pref_change_registrar_.Init(pref_service_);

  // Flag override for testing or demo purposes
  base::FilePath override_sponsored_images_component_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
          switches::kOverrideSponsoredImagesComponentPath));
  if (!override_sponsored_images_component_path.empty()) {
    DVLOG(6)
        << "NTP Sponsored Images test data will be loaded from local path at: "
        << override_sponsored_images_component_path.LossyDisplayName();
    OnSponsoredComponentReady(override_sponsored_images_component_path);
  } else {
    RegisterBackgroundImagesComponent();
    RegisterSponsoredImagesComponent();

    pref_change_registrar_.Add(
        variations::prefs::kVariationsCountry,
        base::BindRepeating(
            &NTPBackgroundImagesService::OnVariationsCountryPrefChanged,
            weak_factory_.GetWeakPtr()));
  }
}

void NTPBackgroundImagesService::StartTearDown() {
  variations_service_ = nullptr;
  component_update_service_ = nullptr;
  pref_service_ = nullptr;
  pref_change_registrar_.RemoveAll();
}

void NTPBackgroundImagesService::MaybeCheckForSponsoredComponentUpdate() {
  // It means component is not ready.
  if (!last_updated_at_) {
    return;
  }

  // If previous update check is missed, do update check now.
  if (base::Time::Now() - *last_updated_at_ >
      features::kSponsoredImagesUpdateCheckAfter.Get()) {
    sponsored_images_update_check_callback_.Run();
  }
}

void NTPBackgroundImagesService::ForceSponsoredComponentUpdate() {
  sponsored_images_component_id_.reset();
  RegisterSponsoredImagesComponent();
}

void NTPBackgroundImagesService::ScheduleNextSponsoredImagesComponentUpdate() {
  const base::Time next_update_check_time =
      base::Time::Now() + features::kSponsoredImagesUpdateCheckAfter.Get();
  sponsored_images_update_check_timer_.Start(
      FROM_HERE, next_update_check_time,
      base::BindOnce(sponsored_images_update_check_callback_));

  VLOG(6)
      << "Scheduled update check for NTP Sponsored Images component with ID "
      << sponsored_images_component_id_.value() << " at "
      << base::TimeFormatFriendlyDateAndTime(next_update_check_time);
}

void NTPBackgroundImagesService::CheckSponsoredImagesComponentUpdate(
    const std::string& component_id) {
  last_updated_at_ = base::Time::Now();

  CheckAndUpdateSponsoredImagesComponent(component_id);

  ScheduleNextSponsoredImagesComponentUpdate();
}

void NTPBackgroundImagesService::RegisterBackgroundImagesComponent() {
  VLOG(6) << "Registering NTP Background Images component";
  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::RegisterSponsoredImagesComponent() {
  const std::string country_code =
      GetVariationsCountryCode(variations_service_);
  const std::optional<SponsoredImagesComponentData> data =
      GetSponsoredImagesComponentData(country_code);
  if (!data) {
    VLOG(6) << "NTP Sponsored Images component is not supported in "
            << country_code;
    return;
  }

  if (sponsored_images_component_id_ == data->component_id.data()) {
    return;
  }

  if (sponsored_images_component_id_) {
    component_update_service_->UnregisterComponent(
        *sponsored_images_component_id_);
  }
  sponsored_images_component_id_ = data->component_id.data();

  VLOG(0) << "Registering NTP Sponsored Images component for " << country_code
          << " with ID " << data->component_id;
  RegisterNTPSponsoredImagesComponent(
      component_update_service_, std::string(data->component_base64_public_key),
      std::string(data->component_id),
      base::StringPrintf("NTP Sponsored Images (%s)", data->region.data()),
      base::BindRepeating(
          &NTPBackgroundImagesService::OnSponsoredComponentReady,
          weak_factory_.GetWeakPtr()));
  // SI component checks update more frequently than other components.
  // By default, browser check update status every 5 hours.
  // However, this background interval is too long for SI. Use 15mins interval.
  sponsored_images_update_check_callback_ = base::BindRepeating(
      &NTPBackgroundImagesService::CheckSponsoredImagesComponentUpdate,
      base::Unretained(this), data->component_id.data());

  last_updated_at_ = base::Time::Now();

  ScheduleNextSponsoredImagesComponentUpdate();
}

void NTPBackgroundImagesService::OnVariationsCountryPrefChanged() {
  RegisterSponsoredImagesComponent();
}

void NTPBackgroundImagesService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void NTPBackgroundImagesService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool NTPBackgroundImagesService::HasObserver(Observer* observer) {
  return observers_.HasObserver(observer);
}

NTPBackgroundImagesData* NTPBackgroundImagesService::GetBackgroundImagesData()
    const {
  if (background_images_data_ && background_images_data_->IsValid()) {
    return background_images_data_.get();
  }

  return nullptr;
}

NTPSponsoredImagesData* NTPBackgroundImagesService::GetSponsoredImagesData(
    bool super_referral,
    bool supports_rich_media) const {
  if (super_referral) {
    return nullptr;
  }

  NTPSponsoredImagesData* const images_data =
      supports_rich_media ? sponsored_images_data_.get()
                          : sponsored_images_data_excluding_rich_media_.get();
  if (!images_data || !images_data->IsValid()) {
    return nullptr;
  }

  return images_data;
}

void NTPBackgroundImagesService::OnComponentReady(
    const base::FilePath& installed_dir) {
  background_images_installed_dir_ = installed_dir;

  VLOG(6) << "NTP Background Images component is ready";

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&HandleComponentData, installed_dir, kNTPManifestFile),
      base::BindOnce(&NTPBackgroundImagesService::OnGetComponentJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnGetComponentJsonData(
    const std::string& json_string) {
  background_images_data_ = std::make_unique<NTPBackgroundImagesData>(
      json_string, background_images_installed_dir_);

  for (auto& observer : observers_) {
    observer.OnBackgroundImagesDataDidUpdate(background_images_data_.get());
  }
}

void NTPBackgroundImagesService::OnSponsoredComponentReady(
    const base::FilePath& installed_dir) {
  VLOG(6) << "NTP Sponsored Images component is ready";
  sponsored_images_installed_dir_ = installed_dir;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&HandleComponentData, installed_dir,
                     kNTPSponsoredManifestFile),
      base::BindOnce(
          &NTPBackgroundImagesService::OnGetSponsoredComponentJsonData,
          weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnGetSponsoredComponentJsonData(
    const std::string& json_string) {
  std::optional<base::Value::Dict> json_value =
      base::JSONReader::ReadDict(json_string);
  if (!json_value) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }
  base::Value::Dict& data = *json_value;

  sponsored_images_data_ = std::make_unique<NTPSponsoredImagesData>(
      data, sponsored_images_installed_dir_);
  sponsored_images_data_excluding_rich_media_ =
      std::make_unique<NTPSponsoredImagesData>(data,
                                               sponsored_images_installed_dir_);
  for (auto& campaign :
       sponsored_images_data_excluding_rich_media_->campaigns) {
    std::erase_if(campaign.creatives, [](const auto& creative) {
      return creative.wallpaper_type == WallpaperType::kRichMedia;
    });
  }
  std::erase_if(
      sponsored_images_data_excluding_rich_media_->campaigns,
      [](const auto& campaign) { return campaign.creatives.empty(); });
  if (!sponsored_images_data_excluding_rich_media_) {
    sponsored_images_data_excluding_rich_media_ =
        std::make_unique<NTPSponsoredImagesData>();
  }

  for (auto& observer : observers_) {
    observer.OnSponsoredContentDidUpdate(data);
  }

  for (auto& observer : observers_) {
    observer.OnSponsoredImagesDataDidUpdate(sponsored_images_data_.get());
  }
}

bool NTPBackgroundImagesService::IsValidSuperReferralComponentInfo(
    const base::Value::Dict& component_info) const {
  return component_info.FindString(kPublicKey) &&
         component_info.FindString(kComponentIDKey) &&
         component_info.FindString(kThemeNameKey);
}

bool NTPBackgroundImagesService::IsSuperReferral() const {
  return false;
}

std::string NTPBackgroundImagesService::GetSuperReferralThemeName() const {
  return "";
}

std::string NTPBackgroundImagesService::GetSuperReferralCode() const {
  return pref_service_->GetString(prefs::kNewTabPageCachedSuperReferralCode);
}

}  // namespace ntp_background_images
