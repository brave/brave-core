/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
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

#if !BUILDFLAG(IS_IOS)
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#endif

namespace ntp_background_images {

namespace {

constexpr char kNTPManifestFile[] = "photo.json";
constexpr char kNTPSponsoredManifestFile[] = "campaigns.json";

constexpr char kNTPSuperReferralMappingTableFile[] = "mapping-table.json";
constexpr char kNTPSuperReferralMappingTableComponentPublicKey[] =
    R"(MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAp7IWv7wzH/KLrxx7BKWOIIUMDylQNzxwM5Fig2WHc16BoMW9Kaya/g17Bpfp0YIvxdcmDBcB9kFALqQLxi1WQfa9d7YxqcmAGUKo407RMwEa6dQVkIPMFz2ZPGSfFgr526gYOqWh3Q4h8oN94qxBLgFyT25SMK5zQDGyq96ntME4MQRNwpDBUv7DDK7Npwe9iE8cBgzYTvf0taAFn2ZZi1RhS0RzpdynucpKosnc0sVBLTXy+HDvnMr+77T48zM0YmpjIh8Qmrp9CNbKzZUsZzNfnHpL9IZnjwQ51EOYdPGX2r1obChVZN19HzpK5scZEMRKoCMfCepWpEkMSIoPzQIDAQAB)";
constexpr char kNTPSuperReferralMappingTableComponentID[] =
    "heplpbhjcbmiibdlchlanmdenffpiibo";
constexpr char kNTPSuperReferralMappingTableComponentName[] =
    "NTP Super Referral mapping table";

std::string GetMappingTableData(const base::FilePath& installed_dir) {
  std::string contents;
  const base::FilePath json_path =
      installed_dir.AppendASCII(kNTPSuperReferralMappingTableFile);
  base::ReadFileToString(json_path, &contents);
  return contents;
}

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
    overridden_component_path_ = true;
    DVLOG(6)
        << "NTP Sponsored Images test data will be loaded from local path at: "
        << override_sponsored_images_component_path.LossyDisplayName();
    OnSponsoredComponentReady(/*is_super_referral=*/false,
                              override_sponsored_images_component_path);
  } else {
    RegisterBackgroundImagesComponent();
    RegisterSponsoredImagesComponent();

    pref_change_registrar_.Add(
        variations::prefs::kVariationsCountry,
        base::BindRepeating(
            &NTPBackgroundImagesService::OnVariationsCountryPrefChanged,
            weak_factory_.GetWeakPtr()));
  }

  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper)) {
    // Flag override for testing or demo purposes
    base::FilePath override_super_referrals_component_path(
        base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
            switches::kOverrideSuperReferralsComponentPath));
    if (!override_super_referrals_component_path.empty()) {
      overridden_component_path_ = true;
      DVLOG(6)
          << "NTP Super Referral test data will be loaded from local path at: "
          << override_super_referrals_component_path.LossyDisplayName();
      OnSponsoredComponentReady(/*is_super_referral=*/false,
                                override_super_referrals_component_path);
    } else {
      CheckSuperReferralComponent();
    }
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
  if (last_update_check_at_.is_null()) {
    return;
  }

  // If previous update check is missed, do update check now.
  if (base::Time::Now() - last_update_check_at_ >
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
  last_update_check_at_ = base::Time::Now();

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
          weak_factory_.GetWeakPtr(), false));
  // SI component checks update more frequently than other components.
  // By default, browser check update status every 5 hours.
  // However, this background interval is too long for SI. Use 15mins
  // interval.
  sponsored_images_update_check_callback_ = base::BindRepeating(
      &NTPBackgroundImagesService::CheckSponsoredImagesComponentUpdate,
      base::Unretained(this), data->component_id.data());

  last_update_check_at_ = base::Time::Now();

  ScheduleNextSponsoredImagesComponentUpdate();
}

void NTPBackgroundImagesService::CheckSuperReferralComponent() {
#if BUILDFLAG(IS_IOS)
  MarkThisInstallIsNotSuperReferralForever();
#else
  const base::Value::Dict& value = pref_service_->GetDict(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  // If we have valid cached SR component info, it means this install is valid
  // SR.
  if (IsValidSuperReferralComponentInfo(value)) {
    RegisterSuperReferralComponent();
    const std::string cached_json_data = pref_service_->GetString(
        prefs::kNewTabPageCachedSuperReferralComponentData);
    if (!cached_json_data.empty()) {
      std::optional<base::Value::Dict> cached_data =
          base::JSONReader::ReadDict(cached_json_data);
      if (!cached_data) {
        return;
      }
      DVLOG(6) << "Initialize Super Referral Data from cache.";
      super_referrals_images_data_ = std::make_unique<NTPSponsoredImagesData>(
          *cached_data, super_referrals_installed_dir_);
    }
    return;
  }

  if (pref_service_
          ->FindPreference(prefs::kNewTabPageCachedSuperReferralComponentInfo)
          ->IsDefaultValue()) {
    // At first fresh launch, we should finish initial component downloading
    // to set initial state properly. But browser could be shutdown accidently
    // before getting it anytime. If this happens, we have to handle this
    // abnormal situation strictly. If not, this install will be act as a non
    // SR install forever. To resolve that situation,
    // |kNewTabPageGetInitialSuperReferralComponentInProgress| introduced.
    // If |kReferralCheckedForPromoCodeFile| or |kReferralInitialization|
    // is true and |kNewTabPageCheckingMappingTableInProgress| is false,
    // this means not first launch. Then, we can mark this install as non-SR.
    // If both (|kReferralCheckedForPromoCodeFile| or
    // |kReferralInitialization|) and
    // |kNewTabPageCheckingMappingTableInProgress| are true, browser had some
    // trouble at first run. If
    // |kNewTabPageGetInitialSuperReferralComponentInProgress| is true, we
    // assume that initial component downloading is in-progress So, We will
    // try initialization again. If referral code is non empty, that means
    // browser is shutdown after getting referal code. In this case, we should
    // start downloading mapping table.
    const bool referral_checked =
        pref_service_->GetBoolean(kReferralCheckedForPromoCodeFile) ||
        pref_service_->GetBoolean(kReferralInitialization);

    if (referral_checked &&
        !pref_service_->GetBoolean(
            prefs::kNewTabPageGetInitialSuperReferralComponentInProgress)) {
      MarkThisInstallIsNotSuperReferralForever();
      DVLOG(6) << "Cached Super Referral Info is clean and Referral Service is "
                  "in initial state. Mark this is not Super Referral install.";
      return;
    }

    // If referral code is empty here, this is fresh launch.
    // If browser is crashed before fetching this install's promo code at
    // fiirst launch, it can be handled here also because code would be empty
    // at this time.
    const std::string code = GetReferralPromoCode();
    if (code.empty()) {
      pref_service_->SetBoolean(
          prefs::kNewTabPageGetInitialSuperReferralComponentInProgress, true);
      MonitorReferralPromoCodeChange();
      return;
    }

    // This below code is for recover above abnormal situation - Shutdown
    // situation before getting map table or getting initial component.
    if (brave::BraveReferralsService::IsDefaultReferralCode(code)) {
      MarkThisInstallIsNotSuperReferralForever();
    } else {
      // If current code is not an default one, let's check it after fetching
      // mapping table.
      DownloadSuperReferralMappingTable();
    }
  }

  DVLOG(6) << "This has invalid component info. In this case, this install is "
              "campaign ended super referral, default referral or non super "
              "referral.";
#endif  // BUILDFLAG(IS_IOS)
}

void NTPBackgroundImagesService::MonitorReferralPromoCodeChange() {
#if !BUILDFLAG(IS_IOS)
  DVLOG(6) << "Monitor for referral promo code change";

  pref_change_registrar_.Add(
      kReferralPromoCode,
      base::BindRepeating(&NTPBackgroundImagesService::OnPreferenceChanged,
                          base::Unretained(this)));
#endif
}

void NTPBackgroundImagesService::OnPreferenceChanged(
  const std::string& pref_name) {
#if !BUILDFLAG(IS_IOS)
  DCHECK_EQ(kReferralPromoCode, pref_name);
  const std::string new_referral_code = GetReferralPromoCode();
  DVLOG(6) << "Got referral promo code: " << new_referral_code;
  DCHECK(!new_referral_code.empty());
  if (brave::BraveReferralsService::IsDefaultReferralCode(new_referral_code)) {
    DVLOG(6) << "This has default referral promo code.";
    MarkThisInstallIsNotSuperReferralForever();
    return;
  }

  DVLOG(6) << "This has non default referral promo code. Let's check this code "
              "is super referral or not after downloading mapping table.";
  DownloadSuperReferralMappingTable();
#endif
}

void NTPBackgroundImagesService::OnVariationsCountryPrefChanged() {
  RegisterSponsoredImagesComponent();
}

void NTPBackgroundImagesService::RegisterSuperReferralComponent() {
  DVLOG(6) << "Registering NTP Super Referral component";

  std::string public_key;
  std::string id;
  std::string theme_name;
  if (initial_super_referrals_component_info_) {
    public_key =
        *initial_super_referrals_component_info_->FindString(kPublicKey);
    id = *initial_super_referrals_component_info_->FindString(kComponentIDKey);
    theme_name =
        *initial_super_referrals_component_info_->FindString(kThemeNameKey);
  } else {
    const base::Value::Dict& value = pref_service_->GetDict(
        prefs::kNewTabPageCachedSuperReferralComponentInfo);
    public_key = *value.FindString(kPublicKey);
    id = *value.FindString(kComponentIDKey);
    theme_name = *value.FindString(kThemeNameKey);
  }

  RegisterNTPSponsoredImagesComponent(
      component_update_service_, public_key, id,
      base::StringPrintf("NTP Super Referral (%s)", theme_name.c_str()),
      base::BindRepeating(
          &NTPBackgroundImagesService::OnSponsoredComponentReady,
          weak_factory_.GetWeakPtr(), true));
}

void NTPBackgroundImagesService::DownloadSuperReferralMappingTable() {
  DVLOG(6) << "Try to download super referral mapping table.";

  if (!component_update_service_) {
    return;
  }

  RegisterNTPSponsoredImagesComponent(
      component_update_service_,
      kNTPSuperReferralMappingTableComponentPublicKey,
      kNTPSuperReferralMappingTableComponentID,
      kNTPSuperReferralMappingTableComponentName,
      base::BindRepeating(
          &NTPBackgroundImagesService::OnMappingTableComponentReady,
          weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnMappingTableComponentReady(
    const base::FilePath& installed_dir) {
  if (!pref_service_
           ->FindPreference(prefs::kNewTabPageCachedSuperReferralComponentInfo)
           ->IsDefaultValue()) {
    DVLOG(6) << "We don't need to handle mapping table update now.";
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&GetMappingTableData, installed_dir),
      base::BindOnce(&NTPBackgroundImagesService::OnGetMappingTableData,
                     weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnGetMappingTableData(
    const std::string& json_string) {
  if (json_string.empty()) {
    DVLOG(6) << "Mapping table is empty.";
    return;
  }

  std::optional<base::Value::Dict> mapping_table_value =
      base::JSONReader::ReadDict(json_string);

  if (!mapping_table_value) {
    DVLOG(6) << "Mapping table is invalid.";
    return;
  }

  DVLOG(6) << "Downloaded valid mapping table.";

  if (base::Value::Dict* value =
          mapping_table_value->FindDict(GetReferralPromoCode())) {
    DVLOG(6) << "This is super referral. Cache SR's referral code";
    initial_super_referrals_component_info_ = std::move(*value);
    RegisterSuperReferralComponent();
    pref_service_->SetString(prefs::kNewTabPageCachedSuperReferralCode,
                             GetReferralPromoCode());
    return;
  }

  DVLOG(6) << "This is non super referral.";
  MarkThisInstallIsNotSuperReferralForever();
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
  const bool is_super_referrals_enabled =
      base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper);
  if (is_super_referrals_enabled) {
    if (super_referral) {
      if (super_referrals_images_data_ &&
          super_referrals_images_data_->IsValid()) {
        return super_referrals_images_data_.get();
      }
      return nullptr;
    }

    // Don't give SI data until we can confirm this is not SR.
    // W/o this check, NTP could show SI images before getting SR data at first
    // run.
    if (pref_service_
            ->FindPreference(prefs::kNewTabPageCachedSuperReferralComponentInfo)
            ->IsDefaultValue()) {
      return nullptr;
    }
  } else {
    if (super_referral) {
      return nullptr;
    }
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
    bool is_super_referral,
    const base::FilePath& installed_dir) {
  if (is_super_referral) {
    DVLOG(6) << "NTP Super Referral component is ready";
    super_referrals_installed_dir_ = installed_dir;
  } else {
    VLOG(6) << "NTP Sponsored Images component is ready";
    sponsored_images_installed_dir_ = installed_dir;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&HandleComponentData, installed_dir,
                     kNTPSponsoredManifestFile),
      base::BindOnce(
          &NTPBackgroundImagesService::OnGetSponsoredComponentJsonData,
          weak_factory_.GetWeakPtr(), is_super_referral));
}

void NTPBackgroundImagesService::OnGetSponsoredComponentJsonData(
    bool is_super_referral,
    const std::string& json_string) {
  std::optional<base::Value::Dict> json_value =
      base::JSONReader::ReadDict(json_string);
  if (!json_value) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }
  base::Value::Dict& data = *json_value;

  if (is_super_referral) {
    pref_service_->SetBoolean(
        prefs::kNewTabPageGetInitialSuperReferralComponentInProgress, false);
    super_referrals_images_data_ = std::make_unique<NTPSponsoredImagesData>(
        data, super_referrals_installed_dir_);
    // |initial_super_referrals_component_info_| has proper data only for
    // initial component downloading. After that, it's empty. In test, it's also
    // empty.
    if (initial_super_referrals_component_info_) {
      pref_service_->SetDict(
          prefs::kNewTabPageCachedSuperReferralComponentInfo,
          std::move(*initial_super_referrals_component_info_));
    }
    pref_service_->SetString(prefs::kNewTabPageCachedSuperReferralComponentData,
                             json_string);
  } else {
    sponsored_images_data_ = std::make_unique<NTPSponsoredImagesData>(
        data, sponsored_images_installed_dir_);
    sponsored_images_data_excluding_rich_media_ =
        std::make_unique<NTPSponsoredImagesData>(
            data, sponsored_images_installed_dir_);
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
  }

  if (is_super_referral && !super_referrals_images_data_->IsValid()) {
    DVLOG(6) << "NTP Super Referral campaign ends.";
    UnRegisterSuperReferralComponent();
    MarkThisInstallIsNotSuperReferralForever();
    return;
  }

  for (auto& observer : observers_) {
    observer.OnSponsoredImagesDataDidUpdate(
        is_super_referral ? super_referrals_images_data_.get()
                          : sponsored_images_data_.get());
  }
}

void NTPBackgroundImagesService::MarkThisInstallIsNotSuperReferralForever() {
  pref_service_->SetDict(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                         base::Value::Dict());
  pref_service_->SetString(prefs::kNewTabPageCachedSuperReferralComponentData,
                           std::string());
  pref_service_->SetString(prefs::kNewTabPageCachedSuperReferralCode,
                           std::string());

  for (auto& observer : observers_) {
    observer.OnSuperReferralCampaignDidEnd();
  }
}

bool NTPBackgroundImagesService::IsValidSuperReferralComponentInfo(
    const base::Value::Dict& component_info) const {
  return component_info.FindString(kPublicKey) &&
         component_info.FindString(kComponentIDKey) &&
         component_info.FindString(kThemeNameKey);
}

void NTPBackgroundImagesService::UnRegisterSuperReferralComponent() {
  if (!component_update_service_) {
    return;
  }

  const base::Value::Dict& value = pref_service_->GetDict(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  const std::string sponsored_referrals_component_id =
      *value.FindString(kComponentIDKey);
  DVLOG(6) << "Unregister NTP Super Referral component";
  component_update_service_->UnregisterComponent(
      sponsored_referrals_component_id);
}

std::string NTPBackgroundImagesService::GetReferralPromoCode() const {
#if BUILDFLAG(IS_IOS)
  return "";
#else
  return pref_service_->GetString(kReferralPromoCode);
#endif
}

bool NTPBackgroundImagesService::IsSuperReferral() const {
  const base::Value::Dict& value = pref_service_->GetDict(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  return base::FeatureList::IsEnabled(
             features::kBraveNTPSuperReferralWallpaper) &&
         IsValidSuperReferralComponentInfo(value);
}

std::string NTPBackgroundImagesService::GetSuperReferralThemeName() const {
  std::string theme_name;
  const base::Value::Dict& value = pref_service_->GetDict(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper) &&
      IsValidSuperReferralComponentInfo(value)) {
    theme_name = *value.FindString(kThemeNameKey);
  }

  return theme_name;
}

std::string NTPBackgroundImagesService::GetSuperReferralCode() const {
  return pref_service_->GetString(prefs::kNewTabPageCachedSuperReferralCode);
}

}  // namespace ntp_background_images
