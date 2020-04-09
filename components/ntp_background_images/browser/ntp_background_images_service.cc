/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"
#include "brave/components/ntp_background_images/browser/sponsored_images_component_data.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/common/chrome_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr int kMappingTableRetryIntervalInHours = 5;
constexpr int kMaxBodySize = 1024 * 1024;
constexpr char kDemoSRPublicKey[] = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzx4iuIBtK/rwFj0JZAfe8ASztzvdPmJNy6HNFxgghzqbfVxDgWCc6zLEYD6ZPwzWcADpcIjd6g4UQ5RobNniYg9leC82K91ed/Jrl/tlTSeYnSe3YUmBl5mkTxiYH6PWC+fomEqx31GrRcOAi6TXdbnPWd/uD/v+hw+6ro7wOPfWySHDC557cZufRTQt8or9ulqr8kcyUIRnH7cF4b4aUEZtHyDuS2qtfB/62OFj50CRBFaxKdhVDnZ72O8YWk0SdCH95vnQFLkVtxwtpABi5deHN79DEmbOWkcfxi6vojg8LZEBFYmayBQD8qBNVI7PBNsCTczIHrZyYDLwmG9BAQIDAQAB";  // NOLINT
constexpr char kDemoSRComponentID[] = "icmficngdmhjligpkfmififbfffcgoef";
constexpr char kDemoSRThemeName[] = "Technikke";

constexpr char kNTPSIManifestFile[] = "photo.json";
constexpr char kNTPSRManifestFile[] = "data.json";

void CacheSuperReferralData(const std::string& data_json,
                            const base::FilePath& installed_dir,
                            const base::FilePath& super_referral_cache_dir) {
  NTPBackgroundImagesData data(data_json,
                               super_referral_cache_dir);

  base::CreateDirectory(super_referral_cache_dir);
  // Cache logo image
  base::CopyFile(installed_dir.Append(data.logo_image_file.BaseName()),
                 data.logo_image_file);
  // Cache topsite favicon images
  for (const auto& top_site : data.top_sites) {
    base::CopyFile(installed_dir.Append(top_site.image_file.BaseName()),
                   top_site.image_file);
  }
  // Cache background images
  for (const auto& background : data.backgrounds) {
    base::CopyFile(installed_dir.Append(background.image_file.BaseName()),
                   background.image_file);
  }
}

// If registered component is for sponsored images wallpaper, it has photo.json
// in |installed_dir|. Otherwise, it has data.json for super referral.
// This methods cache super referral's favicon data because that favicon images
// could be used after campaign ends.
// And return manifest json string.
std::string HandleComponentData(
    const base::FilePath& installed_dir,
    const base::FilePath& super_referral_cache_dir,
    bool is_super_referral) {
  const auto ntp_si_manifest_path =
      installed_dir.AppendASCII(kNTPSIManifestFile);
  const auto ntp_sr_manifest_path =
      installed_dir.AppendASCII(kNTPSRManifestFile);

  base::FilePath json_path;
  if (base::PathExists(ntp_si_manifest_path))
    json_path = ntp_si_manifest_path;
  else if (base::PathExists(ntp_sr_manifest_path))
    json_path = ntp_sr_manifest_path;

  std::string contents;

  if (json_path.empty()) {
    NOTREACHED() << __func__ << ": Can't find valid manifest file in "
                             << installed_dir;
    return contents;
  }

  bool success = base::ReadFileToString(json_path, &contents);
  if (!success || contents.empty()) {
    DVLOG(2) << __func__ << ": cannot read json file " << json_path;
    return contents;
  }

  if (is_super_referral)
    CacheSuperReferralData(contents, installed_dir, super_referral_cache_dir);

  return contents;
}

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("ntp_background_images_service", R"(
        semantics {
          sender:
            "Brave NTP Background Images Service"
          description:
            "Download Super referral component mapping table"
          trigger:
            "When the browser needs to check whether this installation was by
             a 'super referral'"
          data: "component mapping table"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          policy_exception_justification:
            "Not implemented."
        })");
  return network_traffic_annotation_tag;
}

std::string GetSuperReferralMappingTableURL() {
  const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(::switches::kUseGoUpdateDev))
    return kSuperReferralMappingTableDevURL;
  return kSuperReferralMappingTableURL;
}

}  // namespace

NTPBackgroundImagesService::NTPBackgroundImagesService(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_pref,
    const base::FilePath& user_data_dir,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : component_update_service_(cus),
      local_pref_(local_pref),
      super_referral_cache_dir_(
          user_data_dir.AppendASCII("SuperReferralCache")),
      url_loader_factory_(std::move(url_loader_factory)),
      weak_factory_(this) {
}

NTPBackgroundImagesService::~NTPBackgroundImagesService() = default;

void NTPBackgroundImagesService::Init() {
  RestoreCachedTopSitesFaviconList();

  // Flag override for testing or demo purposes
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kNTPSponsoredImagesDataPathForTesting));
  if (!forced_local_path.empty()) {
    test_data_used_ = true;
    DVLOG(2) << __func__ << ": NTP SI test data will be loaded"
             << " from local path at: " << forced_local_path.LossyDisplayName();
    OnComponentReady(false, forced_local_path);
  } else {
    RegisterSponsoredImagesComponent();
  }

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper)) {
    // Flag override for testing or demo purposes
    base::FilePath forced_local_path(
        base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
            switches::kNTPSuperReferralDataPathForTesting));
    if (!forced_local_path.empty()) {
      test_data_used_ = true;
      DVLOG(2) << __func__ << ": NTP SR test data will be loaded"
               << " from local path at: "
               << forced_local_path.LossyDisplayName();
      OnComponentReady(false, forced_local_path);
    } else {
      CheckSuperReferralComponent();
    }
  }
#endif
}

void NTPBackgroundImagesService::RegisterSponsoredImagesComponent() {
  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  const auto& data = GetSponsoredImagesComponentData(
      brave_ads::LocaleHelper::GetCountryCode(locale));
  if (!data) {
    DVLOG(2) << __func__ << ": Not support NTP SI component for " << locale;
    return;
  }

  DVLOG(2) << __func__ << ": Start NTP SI component";
  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      data->component_base64_public_key,
      data->component_id,
      base::StringPrintf("NTP Sponsored Images (%s)", data->region.c_str()),
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr(),
                          false));
}

void NTPBackgroundImagesService::CheckSuperReferralComponent() {
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    RegisterDemoSuperReferralComponent();
    return;
  }

  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  if (IsValidSuperReferralComponentInfo(*value)) {
    RegisterSuperReferralComponent();
    const std::string cached_data = local_pref_->GetString(
        prefs::kNewTabPageCachedSuperReferralComponentData);
    if (!cached_data.empty()) {
      DVLOG(2) << __func__ << ": Initialized SR Data from cache.";
      sr_images_data_.reset(
          new NTPBackgroundImagesData(cached_data,
                                      super_referral_cache_dir_));
    }
    return;
  }

  if (local_pref_->FindPreference(
          prefs::kNewTabPageCachedSuperReferralComponentInfo)->
              IsDefaultValue()) {
    // At first fresh launch, we should finish initial component downloading to
    // set initial state properly.
    // But browser could be shutdown accidently before getting it anytime.
    // If this happens, we have to handle this abnormal situation strictly.
    // If not, this install will be act as a non SR install forever.
    // To resolve that situation,
    // |kNewTabPageGetInitialSRComponentInProgress| introduced.
    // If |kReferralCheckedForPromoCodeFile| is true and
    // |kNewTabPageCheckingMappingTableInProgress| is false, this means not
    // first launch. So, we can mark this install as non-SR.
    // If both |kReferralCheckedForPromoCodeFile| and
    // |kNewTabPageCheckingMappingTableInProgress| are true, browser had some
    // trouble at first launch.
    // If |kNewTabPageGetInitialSRComponentInProgress| is true, we assume that
    // initial component downloading is not finished properly.
    // So, We will try initialization again.
    // If referral code is non empty, that means browser is shutdown after
    // getting referal code. In this case, we should start downloading mapping
    // table.
    // If both |kReferralCheckedForPromoCodeFile| and
    // |kNewTabPageCheckingMappingTableInProgress| are true, browser had some
    // troublewhile getting mapping table at first fresh launch.
    if (local_pref_->GetBoolean(kReferralCheckedForPromoCodeFile) &&
        !local_pref_->GetBoolean(
             prefs::kNewTabPageGetInitialSRComponentInProgress)) {
      MarkThisInstallIsNotSuperReferralForever();
      DVLOG(2) << __func__ << ": Cached SR Info is clean and Referral Service"
                           << " is not in initial state."
                           << " Mark this is not SR install.";
      return;
    }

    // If referral code is empty here, this is fresh launch.
    // If browser is crashed before fetching this install's promo code at fiirst
    // launch, it can be handled here also because code would be empty at this
    // time.
    const std::string code = GetReferralPromoCode();
    if (code.empty()) {
      local_pref_->SetBoolean(
          prefs::kNewTabPageGetInitialSRComponentInProgress,
          true);
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
    return;
  }

  DVLOG(2) << __func__ << ": This has invalid component info.";
  DVLOG(2) << __func__ << ": In this case, this install is campaign ended super"
                       << " referral, default referral or non super referral.";
}

void NTPBackgroundImagesService::MonitorReferralPromoCodeChange() {
  DVLOG(2) << __func__ << ": Monitor referral promo code change";

  pref_change_registrar_.Init(local_pref_);
  pref_change_registrar_.Add(kReferralPromoCode,
      base::BindRepeating(&NTPBackgroundImagesService::OnPreferenceChanged,
      base::Unretained(this)));
}

void NTPBackgroundImagesService::OnPreferenceChanged(
  const std::string& pref_name) {
  DCHECK_EQ(kReferralPromoCode, pref_name);
  const std::string new_referral_code = GetReferralPromoCode();
  DVLOG(2) << __func__ << ": Got referral promo code: "
                       << new_referral_code;
  DCHECK(!new_referral_code.empty());
  if (brave::BraveReferralsService::IsDefaultReferralCode(new_referral_code)) {
    DVLOG(2) << __func__ << ": This has default referral promo code.";
    MarkThisInstallIsNotSuperReferralForever();
    return;
  }

  DVLOG(2) << __func__ << ": This has non default referral promo code."
                       << " Let's check this code is super referral or not"
                       << " after downloading mapping table.";
  DownloadSuperReferralMappingTable();
}

void NTPBackgroundImagesService::RegisterDemoSuperReferralComponent() {
  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      kDemoSRPublicKey,
      kDemoSRComponentID,
      base::StringPrintf("NTP Super Referral (%s)", kDemoSRThemeName),
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr(),
                          true));
}

void NTPBackgroundImagesService::RegisterSuperReferralComponent() {
  DVLOG(2) << __func__ << ": Register NTP SR component";

  std::string public_key;
  std::string id;
  std::string theme_name;
  if (sr_component_info_.is_dict()) {
    public_key = *sr_component_info_.FindStringKey(kPublicKey);
    id = *sr_component_info_.FindStringKey(kComponentID);
    theme_name = *sr_component_info_.FindStringKey(kThemeName);
  } else {
    const auto* value = local_pref_->Get(
        prefs::kNewTabPageCachedSuperReferralComponentInfo);
    public_key = *value->FindStringKey(kPublicKey);
    id = *value->FindStringKey(kComponentID);
    theme_name = *value->FindStringKey(kThemeName);
  }

  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      public_key, id,
      base::StringPrintf("NTP Super Referral (%s)",
                         theme_name.c_str()),
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr(),
                          true));
}

void NTPBackgroundImagesService::DownloadSuperReferralMappingTable() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(GetSuperReferralMappingTableURL());
  request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;
  loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&NTPBackgroundImagesService::OnGetMappingTableData,
                     base::Unretained(this)),
      kMaxBodySize);
}

void NTPBackgroundImagesService::ScheduleMappingTabRetryTimer() {
  mapping_table_retry_timer_ = std::make_unique<base::OneShotTimer>();
  mapping_table_retry_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromHours(kMappingTableRetryIntervalInHours),
      base::BindOnce(
          &NTPBackgroundImagesService::DownloadSuperReferralMappingTable,
          base::Unretained(this)));
}

void NTPBackgroundImagesService::OnGetMappingTableData(
    std::unique_ptr<std::string> json_string) {
  if (!json_string) {
    DVLOG(2) << __func__ << ": Failed to fetch mapping table.";
    DVLOG(2) << __func__ << ": Schedule re-trying mapping table download.";
    ScheduleMappingTabRetryTimer();
    return;
  }

  base::Optional<base::Value> mapping_table_value =
      base::JSONReader::Read(*json_string);

  if (!mapping_table_value) {
    DVLOG(2) << __func__ << ": has invalid mapping table.";
    return;
  }

  if (!mapping_table_value->is_dict()) {
    DVLOG(2) << __func__ << ": Mapping table is empty.";
    return;
  }

  DVLOG(2) << __func__ << ": Downloaded valid mapping table.";

  if (const base::Value* value =
          mapping_table_value->FindDictKey(GetReferralPromoCode())) {
    DVLOG(2) << __func__
             << ": This is super referral. Cache SR's referral code";
    sr_component_info_ = value->Clone();
    RegisterSuperReferralComponent();
    local_pref_->SetString(prefs::kNewTabPageCachedSuperReferralCode,
                           GetReferralPromoCode());
    return;
  }

  DVLOG(2) << __func__ << ": This is non super referral.";
  MarkThisInstallIsNotSuperReferralForever();
}

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
NTPBackgroundImagesService::GetBackgroundImagesData(bool super_referral) const {
  const bool is_sr_enabled =
      base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper);
  if (is_sr_enabled) {
    if (super_referral) {
      if (sr_images_data_ && sr_images_data_->IsValid())
        return sr_images_data_.get();
      return nullptr;
    }

    // Don't give SI data until we can confirm this is not SR.
    // W/o this check, NTP could show SI images before getting SR data at first
    // run.
    if (local_pref_->FindPreference(
          prefs::kNewTabPageCachedSuperReferralComponentInfo)->
              IsDefaultValue())
      return nullptr;
  }

  if (si_images_data_ && si_images_data_->IsValid())
    return si_images_data_.get();

  return nullptr;
}

void NTPBackgroundImagesService::OnComponentReady(
    bool is_super_referral,
    const base::FilePath& installed_dir) {
  if (is_super_referral)
    sr_installed_dir_ = installed_dir;
  else
    si_installed_dir_ = installed_dir;

  DVLOG(2) << __func__ << (is_super_referral ? ": NPT SR Component is ready"
                                             : ": NTP SI Component is ready");

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&HandleComponentData, installed_dir,
                     super_referral_cache_dir_, is_super_referral),
      base::BindOnce(&NTPBackgroundImagesService::OnGetComponentJsonData,
                     weak_factory_.GetWeakPtr(),
                     is_super_referral));
}

void NTPBackgroundImagesService::OnGetComponentJsonData(
    bool is_super_referral,
    const std::string& json_string) {
  if (is_super_referral) {
    local_pref_->SetBoolean(
          prefs::kNewTabPageGetInitialSRComponentInProgress,
          false);
    sr_images_data_.reset(
        new NTPBackgroundImagesData(json_string,
                                    super_referral_cache_dir_));
    // In test, |sr_component_info_| could be empty.
    if (sr_component_info_.is_dict()) {
      local_pref_->Set(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                       sr_component_info_);
    }
    if (local_pref_->FindPreference(
            prefs::kNewTabPageCachedSuperReferralFaviconList)->
                IsDefaultValue() &&
        sr_images_data_->IsValid()) {
      // This is done only once because super referral will have same top sites
      // list forever.
      CacheTopSitesFaviconList();
    }
    local_pref_->SetString(prefs::kNewTabPageCachedSuperReferralComponentData,
                           json_string);
  } else {
    si_images_data_.reset(new NTPBackgroundImagesData(json_string,
                                                      si_installed_dir_));
  }

  for (auto& observer : observer_list_) {
    observer.OnUpdated(is_super_referral ? sr_images_data_.get()
                                         : si_images_data_.get());
  }

  if (is_super_referral && !sr_images_data_->IsValid()) {
    DVLOG(2) << __func__ << ": NTP SR campaign ends.";
    UnRegisterSuperReferralComponent();
    MarkThisInstallIsNotSuperReferralForever();
  }
}

void NTPBackgroundImagesService::MarkThisInstallIsNotSuperReferralForever() {
  local_pref_->Set(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                   base::Value(base::Value::Type::DICTIONARY));
  local_pref_->SetString(prefs::kNewTabPageCachedSuperReferralComponentData,
                         std::string());
  local_pref_->SetString(prefs::kNewTabPageCachedSuperReferralCode,
                         std::string());
}

void NTPBackgroundImagesService::CacheTopSitesFaviconList() {
  DCHECK(sr_images_data_->IsValid());
  DCHECK(local_pref_->FindPreference(
            prefs::kNewTabPageCachedSuperReferralFaviconList)->
                IsDefaultValue());
  base::Value list(base::Value::Type::LIST);

  for (const auto& top_site : sr_images_data_->top_sites) {
    const std::string file_path = super_referral_cache_dir_.Append(
        top_site.image_file.BaseName()).AsUTF8Unsafe();
    list.Append(file_path);
    cached_top_site_favicon_list_.push_back(file_path);
  }

  local_pref_->Set(prefs::kNewTabPageCachedSuperReferralFaviconList, list);
}

void NTPBackgroundImagesService::RestoreCachedTopSitesFaviconList() {
  if (local_pref_->FindPreference(
            prefs::kNewTabPageCachedSuperReferralFaviconList)->
                IsDefaultValue()) {
    return;
  }

  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralFaviconList);
  for (const auto& file : value->GetList())
      cached_top_site_favicon_list_.push_back(file.GetString());
}

std::vector<std::string>
NTPBackgroundImagesService::GetCachedTopSitesFaviconList() const {
  return cached_top_site_favicon_list_;
}

void NTPBackgroundImagesService::UnRegisterSuperReferralComponent() {
  if (!component_update_service_)
    return;

  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  DCHECK(value);
  const std::string sr_component_id = *value->FindStringKey(kComponentID);
  DVLOG(2) << __func__ << ": Unregister NTP SR component";
  component_update_service_->UnregisterComponent(sr_component_id);
}

std::string NTPBackgroundImagesService::GetReferralPromoCode() const {
  return local_pref_->GetString(kReferralPromoCode);
}

void NTPBackgroundImagesService::InitializeWebUIDataSource(
    content::WebUIDataSource* html_source) {
  html_source->AddString("superReferralThemeName", GetSuperReferralThemeName());
}

bool NTPBackgroundImagesService::IsSuperReferral() const {
  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  return
      base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper) &&
      IsValidSuperReferralComponentInfo(*value);
}

std::string NTPBackgroundImagesService::GetSuperReferralThemeName() const {
  std::string theme_name;
  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper) &&
      IsValidSuperReferralComponentInfo(*value)) {
    theme_name = *value->FindStringKey(kThemeName);
  }

  return theme_name;
}

std::string NTPBackgroundImagesService::GetSuperReferralCode() const {
  return local_pref_->GetString(prefs::kNewTabPageCachedSuperReferralCode);
}

}  // namespace ntp_background_images
