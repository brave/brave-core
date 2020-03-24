/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <algorithm>
#include <utility>
#include <vector>

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
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr int kMaxBodySize = 1024 * 1024;

constexpr char kDemoSuperReferrerCode[] = "TECHNIK";

constexpr char kNTPSIManifestFile[] = "photo.json";
constexpr char kNTPSRManifestFile[] = "data.json";

constexpr char kPublicKey[] = "publicKey";
constexpr char kComponentID[] = "componentID";
constexpr char kCompanyName[] = "companyName";

void DeleteComponentFolder(const base::FilePath& component_dir) {
  base::DeleteFile(component_dir, true);
}

// If registered component is for sponsored images wallpaper, it has photo.json
// in |installed_dir|. Otherwise, it has data.json for super referrer.
std::string ReadComponentJsonData(const base::FilePath& installed_dir) {
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
  if (!success || contents.empty())
    DVLOG(2) << __func__ << ": cannot read json file " << json_path;
  return contents;
}

bool IsValidSuperReferrerComponentInfo(const base::Value& component_info) {
  if (!component_info.is_dict())
    return false;

  if (!component_info.FindStringKey(kPublicKey))
    return false;
  if (!component_info.FindStringKey(kComponentID))
    return false;
  if (!component_info.FindStringKey(kCompanyName))
    return false;

  return true;
}

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("ntp_background_images_service", R"(
        semantics {
          sender:
            "Brave NTP Background Images Service"
          description:
            "Download Super referrer component mapping table"
          trigger:
            "When the browser needs to check whether this installation was by
             a 'super referrer'"
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

std::string GetSuperReferrerMappingTableURL() {
  const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(::switches::kUseGoUpdateDev))
    return kSuperReferrerMappingTableDevURL;
  return kSuperReferrerMappingTableURL;
}

}  // namespace

NTPBackgroundImagesService::NTPBackgroundImagesService(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_pref,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : component_update_service_(cus),
      local_pref_(local_pref),
      url_loader_factory_(std::move(url_loader_factory)),
      weak_factory_(this) {
}

NTPBackgroundImagesService::~NTPBackgroundImagesService() = default;

void NTPBackgroundImagesService::Init() {
  if (UseLocalTestData())
    return;

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferrerWallpaper)) {
    // Download mapping table first. Then determine which component should be
    // registered.
    DownloadSuperReferrerMappingTable();
    return;
  }
#endif

  StartSponsoredImagesComponent();
}

void NTPBackgroundImagesService::DownloadSuperReferrerMappingTable() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(GetSuperReferrerMappingTableURL());
  request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES;
  loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&NTPBackgroundImagesService::OnGetMappingTableData,
                     base::Unretained(this)),
      kMaxBodySize);
}

void NTPBackgroundImagesService::MonitorReferralPromoCodeChange() {
  // If referrals code is not empty here, it means referral code is fetched and
  // set before this class is initialized. In this case, we should check this
  // referral code with mapping table. So, ask mapping table download.
  pref_change_registrar_.Init(local_pref_);
  pref_change_registrar_.Add(kReferralPromoCode,
      base::BindRepeating(&NTPBackgroundImagesService::OnPreferenceChanged,
      base::Unretained(this)));
}

bool NTPBackgroundImagesService::IsAlreadyRegistered(
    const std::string& component_id) {
  std::vector<std::string> component_ids;
  if (!component_update_service_)
    return false;

  component_ids = component_update_service_->GetComponentIDs();
  return std::find(component_ids.begin(),
                   component_ids.end(),
                   component_id) != component_ids.end();
}

void NTPBackgroundImagesService::UnRegisterSponsoredImagesComponentIfRunning() {
  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  const auto& data = GetSponsoredImagesComponentData(
      brave_ads::LocaleHelper::GetCountryCode(locale));
  if (!data)
    return;

  if (!component_update_service_)
    return;

  if (IsAlreadyRegistered(data->component_id)) {
    component_update_service_->UnregisterComponent(data->component_id);
    DVLOG(2) << __func__ << ": Unregister SI Component.";
  }
}

void NTPBackgroundImagesService::StartSuperReferrerComponent(
    const std::string& super_referral_code) {
  DCHECK(mapping_table_value_.is_dict());

  auto* value = mapping_table_value_.FindDictKey(super_referral_code);
  if (!value) {
    NOTREACHED();
    return;
  }

  is_super_referrer_lastly_asked_component_ = true;
  if (IsAlreadyRegistered(*value->FindStringKey(kComponentID)))
    return;

  local_pref_->Set(prefs::kNewTabPageCachedSuperReferrerComponentInfo, *value);

  UnRegisterSponsoredImagesComponentIfRunning();

  DVLOG(2) << __func__ << ": Start NTP SR component";
  DCHECK(IsValidSuperReferrerComponentInfo(*value));
  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      *value->FindStringKey(kPublicKey),
      *value->FindStringKey(kComponentID),
      base::StringPrintf("NTP Super Referrer (%s)",
                         value->FindStringKey(kCompanyName)->c_str()),
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr(),
                          true));
}

void NTPBackgroundImagesService::StartSponsoredImagesComponent() {
  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  const auto& data = GetSponsoredImagesComponentData(
      brave_ads::LocaleHelper::GetCountryCode(locale));
  if (!data)
    return;

  is_super_referrer_lastly_asked_component_ = false;
  if (IsAlreadyRegistered(data->component_id))
    return;

  // If SI is not running, unregister SR if it is running now.
  UnRegisterSuperReferrerComponentIfRunning(GetCachedReferralPromoCode());

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

void NTPBackgroundImagesService::UnRegisterSuperReferrerComponentIfRunning(
    const std::string& referral_code) {
  if (!mapping_table_value_.is_dict())
    return;

  if (!component_update_service_)
    return;

  if (const base::Value* value =
          mapping_table_value_.FindDictKey(referral_code)) {
    const std::string sr_component_id = *value->FindStringKey(kComponentID);
    if (IsAlreadyRegistered(sr_component_id)) {
      DVLOG(2) << __func__ << ": Unregister NTP SR component";
      component_update_service_->UnregisterComponent(sr_component_id);
    }
  }
}

void NTPBackgroundImagesService::OnGetMappingTableData(
    std::unique_ptr<std::string> json_string) {
  bool has_valid_mapping_table = true;
  base::Optional<base::Value> mapping_table_value;
  if (!json_string) {
    has_valid_mapping_table = false;
  } else {
    mapping_table_value = base::JSONReader::Read(*json_string);
    if (!mapping_table_value)
      has_valid_mapping_table = false;
  }

  if (!has_valid_mapping_table) {
    DVLOG(2) << __func__ << ": Failed to fetch mapping table."
                         << " Start NTP SR or SI.";
    const auto* value = local_pref_->Get(
        prefs::kNewTabPageCachedSuperReferrerComponentInfo);
    // If we don't get proper mapping table by network error or any reason,
    // start SR if this install comes from valid super referrer install.
    // If not, start SI component.
    if (IsValidSuperReferrerComponentInfo(*value)) {
      // Generate temporal mapping table that only includes current install
      // because rest of the code assumes |mapping_table_value_| is valid.
      const std::string cached_promo_code = GetCachedReferralPromoCode();
      mapping_table_value_ = base::Value(base::Value::Type::DICTIONARY);
      mapping_table_value_.SetKey(cached_promo_code, value->Clone());
      StartSuperReferrerComponent(cached_promo_code);
      return;
    }
    StartSponsoredImagesComponent();
    return;
  }

  if (!mapping_table_value->is_dict()) {
    DVLOG(2) << __func__ << ": We don't have any super referrer."
                         << " Mapping table is empty.";
    StartSponsoredImagesComponent();
    return;
  }

  DVLOG(2) << __func__ << ": Downloaded valid mapping table.";
  mapping_table_value_ = std::move(*mapping_table_value);

  // Sync with referral code if it's not empty.
  if (!GetReferralPromoCode().empty()) {
    local_pref_->SetString(prefs::kNewTabPageCachedReferralPromoCode,
                           GetReferralPromoCode());
  }

  // From now on, we need to know kReferralPromoCode changing.
  MonitorReferralPromoCodeChange();
  DetermineTargetComponent();

  // After getting valid mapping table, we can start cleanup.
  CleanUp();
}

void NTPBackgroundImagesService::DetermineTargetComponent() {
  const std::string cached_promo_code = GetCachedReferralPromoCode();
  // There are two cases if |cached_promo_code| is empty.
  // The one is this running is first run so promo code is not fetched yet by
  // BraveReferralsService. In this case, we can get promocode when
  // OnPreferenceChanged() is changed.
  // The other is not first run and the code was already cleared because of
  // 90 days passed. In this case, OnPreference will not be called.
  // To make logic simple, start NTP SI component proactively. If this method is
  // called again by OnPreferenceChanged(), we can judge which component should
  // be registered. If not, it's good to use already registered SI component.
  if (cached_promo_code.empty()) {
    DVLOG(2) << __func__ << ": Cached code empty. Start NTP SI component.";
    DVLOG(2) << __func__
             << ": If this is super referral, this SI component will be"
             << " unregistered and NTP SR component will be registered later.";
    StartSponsoredImagesComponent();
    return;
  }

  // If cached one is default promo code, start SI component.
  if (brave::BraveReferralsService::IsDefaultReferralCode(cached_promo_code)) {
    DVLOG(2) << __func__ << ": Default referral code is detected.";
    StartSponsoredImagesComponent();
    return;
  }

  // Current one is not default promo code.
  // Then, check current promo code is super referrer or not.
  if (IsSuperReferrerCode(cached_promo_code)) {
    DVLOG(2) << __func__
             << ": Super referrer (" << cached_promo_code << ") is detected";
    StartSuperReferrerComponent(cached_promo_code);
    return;
  }

  DVLOG(2) << __func__
           << ": Non super referrer (" << cached_promo_code << ") is detected";
  StartSponsoredImagesComponent();
}

void NTPBackgroundImagesService::CleanUp() {
  if (IsSuperReferrerCode(GetCachedReferralPromoCode()))
    return;

  // If browser gets invalid NTP SR component during the runtime, it's cleaned
  // up by unregistering. If not, we should cleanup assets explicitely.
  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferrerComponentInfo);
  if (IsValidSuperReferrerComponentInfo(*value)) {
    const std::string id = *value->FindStringKey(kComponentID);
    base::FilePath component_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &component_dir);
    component_dir = component_dir.AppendASCII(id);
    base::PostTask(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&DeleteComponentFolder, component_dir));
    local_pref_->ClearPref(prefs::kNewTabPageCachedReferralPromoCode);
    local_pref_->ClearPref(prefs::kNewTabPageCachedSuperReferrerComponentInfo);
    DVLOG(2) << __func__ << ": Delete ended compaign's component folder at "
                         << component_dir;
  }
}

bool NTPBackgroundImagesService::IsSuperReferrerCode(
    const std::string& referral_code) {
  if (!mapping_table_value_.is_dict())
    return false;

  return mapping_table_value_.FindDictKey(referral_code) != nullptr;
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
NTPBackgroundImagesService::GetBackgroundImagesData() const {
  if (images_data_ && images_data_->IsValid())
    return images_data_.get();

  return nullptr;
}

void NTPBackgroundImagesService::OnComponentReady(
    bool is_super_referrer,
    const base::FilePath& installed_dir) {
  // Early return for test data because any components are not used when test
  // data is used.
  if (test_data_used_) {
    GetComponentJsonData(installed_dir);
    return;
  }

  // If we request component(1) registration after asking component(2), we can't
  // guarantee component(1) is delivered first than 2.
  // If (1) is delivered lately, we should ignore it and un register it. In this
  // case, we should not update |images_data_| with it.
  // Usually, we tries to unregister other type of component when a component
  // is registered. However, both components could be in ready state when both
  // are registered in a row. In this case, component(1) should be unregistered.
  // One example is - we register NTP SI if we don't have cached referall code.
  // When referral service fetched a code, we can register NTP SR if it's super
  // referral code.
  if (is_super_referrer != is_super_referrer_lastly_asked_component_) {
    const std::string code = GetCachedReferralPromoCode();
    is_super_referrer ? UnRegisterSuperReferrerComponentIfRunning(code)
                      : UnRegisterSponsoredImagesComponentIfRunning();
    return;
  }

  GetComponentJsonData(installed_dir);
}

void NTPBackgroundImagesService::GetComponentJsonData(
  const base::FilePath& installed_dir) {
  installed_dir_ = installed_dir;
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadComponentJsonData, installed_dir_),
      base::BindOnce(&NTPBackgroundImagesService::OnGetComponentJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnGetComponentJsonData(
    const std::string& json_string) {
  const bool was_super_referrer =
      images_data_ && images_data_->IsSuperReferrer();
  images_data_.reset(new NTPBackgroundImagesData(json_string, installed_dir_));
  NotifyObservers();

  if (was_super_referrer && !images_data_->IsValid()) {
    DVLOG(2) << __func__ << ": NTP SR campaign ends. Start NTP SI.";
    // If super referrer component is not valid, it means its campaign is ended.
    // Start NTP SI component now. And, we don't need NPT SR component anymore.
    // StartSponsoredImagesComponent() will unregister currently registered SR
    // first and then start SI component.
    StartSponsoredImagesComponent();
    // Clear cached referral promo code.
    local_pref_->ClearPref(prefs::kNewTabPageCachedReferralPromoCode);
    local_pref_->ClearPref(prefs::kNewTabPageCachedSuperReferrerComponentInfo);
    // Also, that SR will be removed from mapping table. So, this install can't
    // register that SR anymore regardless of current kReferralPromoCode after
    // next launches.
  }
}

void NTPBackgroundImagesService::NotifyObservers() {
  for (auto& observer : observer_list_) {
    observer.OnUpdated(images_data_.get());
  }
}

void NTPBackgroundImagesService::OnPreferenceChanged(
  const std::string& pref_name) {
  DCHECK_EQ(kReferralPromoCode, pref_name);
  const std::string new_referral_code = GetReferralPromoCode();
  DVLOG(2) << __func__ << ": Got referral promo code: "
                       << new_referral_code;
  // If referral_code is not empty, it means referral code is fetched at first
  // run or referral code is changed by another install.
  if (!new_referral_code.empty()) {
    const std::string previous_referral_code = GetCachedReferralPromoCode();
    // Sync with referral code if it's not empty.
    local_pref_->SetString(prefs::kNewTabPageCachedReferralPromoCode,
                           new_referral_code);

    // If previous referrer is super referrer and current referrer another
    // super referrer, we should unregister previous one.
    if (previous_referral_code != new_referral_code) {
      DVLOG(2) << __func__ << ": Referrer was changed from "
                           << previous_referral_code << " to "
                           << new_referral_code;
      UnRegisterSuperReferrerComponentIfRunning(previous_referral_code);
    }

    DetermineTargetComponent();
  }
}

std::string NTPBackgroundImagesService::GetReferralPromoCode() const {
  return local_pref_->GetString(kReferralPromoCode);
}

std::string NTPBackgroundImagesService::GetCachedReferralPromoCode() const {
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo))
    return kDemoSuperReferrerCode;

  return local_pref_->GetString(prefs::kNewTabPageCachedReferralPromoCode);
}

bool NTPBackgroundImagesService::UseLocalTestData() {
  // Flag override for testing or demo purposes
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kNTPBrandedDataPathForTesting));
  if (!forced_local_path.empty()) {
    test_data_used_ = true;
    DVLOG(2) << __func__ << ": NTP background image package will be loaded"
             << " from local path at: " << forced_local_path.LossyDisplayName();
    // Passing |true| doesn't have much meaning because OnComponentReady()
    // will handle properly test data regardless of passed value.
    OnComponentReady(true, forced_local_path);
    return true;
  }

  return false;
}

}  // namespace ntp_background_images
