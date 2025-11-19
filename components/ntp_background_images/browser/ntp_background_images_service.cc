/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <memory>

#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_update_util.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_images_component_data.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/service/variations_service_utils.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ntp_background_images {

namespace {

constexpr char kNTPManifestFile[] = "photo.json";
constexpr char kNTPSponsoredManifestFile[] = "campaigns.json";

constexpr char kNewTabPageCachedSuperReferralComponentInfo[] =
    "brave.new_tab_page.cached_super_referral_component_info";
constexpr char kNewTabPageCachedSuperReferralComponentData[] =
    "brave.new_tab_page.cached_super_referral_component_data";
constexpr char kNewTabPageGetInitialSuperReferralComponentInProgress[] =
    "brave.new_tab_page.get_initial_sr_component_in_progress";
constexpr char kNewTabPageCachedSuperReferralCode[] =
    "brave.new_tab_page.cached_referral_code";

// If registered component is for sponsored content, it has
// `kNTPSponsoredManifestFile` in |installed_dir|.
std::string HandleComponentData(const base::FilePath& installed_dir,
                                const std::string& manifest_file) {
  const base::FilePath file_path = installed_dir.AppendASCII(manifest_file);

  std::string contents;
  const bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    SCOPED_CRASH_KEY_BOOL("Issue50267", "success", success);
    SCOPED_CRASH_KEY_BOOL("Issue50267", "empty_contents", contents.empty());
    SCOPED_CRASH_KEY_BOOL("Issue50267", "path_exists",
                          base::PathExists(file_path));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "filename",
                              file_path.BaseName().AsUTF8Unsafe());
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason", "Invalid JSON");
    DUMP_WILL_BE_NOTREACHED();
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
void NTPBackgroundImagesService::RegisterLocalStatePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 10/2025
  registry->RegisterDictionaryPref(kNewTabPageCachedSuperReferralComponentInfo);
  registry->RegisterStringPref(kNewTabPageCachedSuperReferralComponentData,
                               std::string());
  registry->RegisterStringPref(kNewTabPageCachedSuperReferralCode,
                               std::string());
  registry->RegisterBooleanPref(
      kNewTabPageGetInitialSuperReferralComponentInProgress, false);
}

// static
void NTPBackgroundImagesService::MigrateObsoleteLocalStatePrefs(
    PrefService* local_state) {
  // Added 10/2025
  local_state->ClearPref(kNewTabPageCachedSuperReferralComponentInfo);
  local_state->ClearPref(kNewTabPageCachedSuperReferralComponentData);
  local_state->ClearPref(kNewTabPageCachedSuperReferralCode);
  local_state->ClearPref(kNewTabPageGetInitialSuperReferralComponentInProgress);
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
    if (sponsored_images_update_check_callback_) {
      sponsored_images_update_check_callback_.Run();
    }
  }
}

void NTPBackgroundImagesService::ForceSponsoredComponentUpdate() {
  sponsored_images_component_id_.reset();
  RegisterSponsoredImagesComponent();
}

void NTPBackgroundImagesService::ScheduleNextSponsoredImagesComponentUpdate() {
  if (!sponsored_images_update_check_callback_) {
    return;
  }

  const base::Time next_update_check_time =
      base::Time::Now() + features::kSponsoredImagesUpdateCheckAfter.Get();
  sponsored_images_update_check_timer_.Start(
      FROM_HERE, next_update_check_time,
      base::BindOnce(sponsored_images_update_check_callback_));

  if (sponsored_images_component_id_) {
    VLOG(6)
        << "Scheduled update check for NTP Sponsored Images component with ID "
        << *sponsored_images_component_id_ << " at "
        << base::TimeFormatFriendlyDateAndTime(next_update_check_time);
  }
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
  const std::string variations_country_code =
      GetVariationsCountryCode(variations_service_);
  const std::optional<SponsoredImagesComponentInfo> sponsored_images_component =
      GetSponsoredImagesComponent(variations_country_code);
  if (!sponsored_images_component) {
    // Unsupported.
    return;
  }

  if (sponsored_images_component_id_ == sponsored_images_component->id) {
    // Already registered.
    return;
  }

  if (sponsored_images_component_id_) {
    // Unregister previous component.
    component_update_service_->UnregisterComponent(
        *sponsored_images_component_id_);
  }
  sponsored_images_component_id_ = sponsored_images_component->id;

  VLOG(0) << "Registering NTP Sponsored Images component for "
          << variations_country_code << " with ID "
          << *sponsored_images_component_id_;
  RegisterNTPSponsoredImagesComponent(
      component_update_service_,
      std::string(sponsored_images_component->public_key_base64),
      *sponsored_images_component_id_,
      absl::StrFormat("NTP Sponsored Images (%s)", variations_country_code),
      base::BindRepeating(
          &NTPBackgroundImagesService::OnSponsoredComponentReady,
          weak_factory_.GetWeakPtr()));
  // SI component checks update more frequently than other components.
  // By default, browser check update status every 5 hours.
  // However, this background interval is too long for SI. Use 15mins interval.
  sponsored_images_update_check_callback_ = base::BindRepeating(
      &NTPBackgroundImagesService::CheckSponsoredImagesComponentUpdate,
      weak_factory_.GetWeakPtr(), *sponsored_images_component_id_);

  last_updated_at_ = base::Time::Now();

  ScheduleNextSponsoredImagesComponentUpdate();
}

void NTPBackgroundImagesService::OnVariationsCountryPrefChanged() {
  if (sponsored_images_component_id_) {
    // Re-register the Sponsored Images component when the country preference
    // changes. Defer first registration until ads service initialization
    // completes, to prevent race conditions.
    RegisterSponsoredImagesComponent();
  }
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
    bool supports_rich_media) const {
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
  std::optional<base::Value::Dict> json_value = base::JSONReader::ReadDict(
      json_string, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!json_value) {
    SCOPED_CRASH_KEY_STRING64("Issue50267", "variations_country_code",
                              GetVariationsCountryCode(variations_service_));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "json_string", json_string);
    SCOPED_CRASH_KEY_BOOL("Issue50267", "empty_json", json_string.empty());
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason", "Invalid JSON");
    base::debug::DumpWithoutCrashing();
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

}  // namespace ntp_background_images
