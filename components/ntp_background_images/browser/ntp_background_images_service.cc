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

constexpr int kMaxBodySize = 1024 * 1024;

constexpr char kDemoSuperReferralCode[] = "TECHNIK";

constexpr char kNTPSIManifestFile[] = "photo.json";
constexpr char kNTPSRManifestFile[] = "data.json";

constexpr char kPublicKey[] = "publicKey";
constexpr char kComponentID[] = "componentID";
constexpr char kThemeName[] = "themeName";

void CacheFaviconImages(const std::string& data_json,
                        const base::FilePath& installed_dir,
                        const base::FilePath& top_sites_favicon_cache_dir) {
  NTPBackgroundImagesData data(data_json,
                               installed_dir,
                               top_sites_favicon_cache_dir);
  if (base::PathExists(top_sites_favicon_cache_dir))
    return;

  base::CreateDirectory(top_sites_favicon_cache_dir);
  for (const auto& top_site : data.top_sites) {
    base::CopyFile(installed_dir.Append(top_site.image_file.BaseName()),
                   top_site.image_file);
  }
}

// If registered component is for sponsored images wallpaper, it has photo.json
// in |installed_dir|. Otherwise, it has data.json for super referral.
// This methods cache super referral's favicon data because that favicon images
// could be used after campaign ends.
// And return manifest json string.
std::string HandleComponentData(
    const base::FilePath& installed_dir,
    const base::FilePath& top_sites_favicon_cache_dir,
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
    CacheFaviconImages(contents, installed_dir, top_sites_favicon_cache_dir);

  return contents;
}

bool IsValidSuperReferralComponentInfo(const base::Value& component_info) {
  if (!component_info.is_dict())
    return false;

  if (!component_info.FindStringKey(kPublicKey))
    return false;
  if (!component_info.FindStringKey(kComponentID))
    return false;
  if (!component_info.FindStringKey(kThemeName))
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
      top_sites_favicon_cache_dir_(
          user_data_dir.AppendASCII("SuperReferralCache")),
      url_loader_factory_(std::move(url_loader_factory)),
      weak_factory_(this) {
}

NTPBackgroundImagesService::~NTPBackgroundImagesService() = default;

bool NTPBackgroundImagesService::UseLocalSponsoredImagesestData() {
  // Flag override for testing or demo purposes
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kNTPSponsoredImagesDataPathForTesting));
  if (!forced_local_path.empty()) {
    test_data_used_ = true;
    DVLOG(2) << __func__ << ": NTP SI test data will be loaded"
             << " from local path at: " << forced_local_path.LossyDisplayName();
    OnComponentReady(false, forced_local_path);
    return true;
  }

  return false;
}

bool NTPBackgroundImagesService::UseLocalSuperReferralTestData() {
  // Flag override for testing or demo purposes
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kNTPSuperReferralDataPathForTesting));
  if (!forced_local_path.empty()) {
    test_data_used_ = true;
    DVLOG(2) << __func__ << ": NTP SR test data will be loaded"
             << " from local path at: " << forced_local_path.LossyDisplayName();
    OnComponentReady(false, forced_local_path);
    return true;
  }

  return false;
}

void NTPBackgroundImagesService::Init() {
  RestoreCachedTopSitesFaviconList();

  if (!UseLocalSponsoredImagesestData())
    RegisterSponsoredImagesComponent();

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper)) {
    if (!UseLocalSuperReferralTestData())
      CheckSuperReferralComponent();
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
  if (local_pref_->FindPreference(
          prefs::kNewTabPageCachedSuperReferralComponentInfo)->
              IsDefaultValue()) {
    MonitorReferralPromoCodeChange();
    return;
  }

  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  if (IsValidSuperReferralComponentInfo(*value)) {
    RegisterSuperReferralComponent();
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
  // Referral promo code could be empty after 90 days from first run.
  //
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

void NTPBackgroundImagesService::RegisterSuperReferralComponent() {
  DVLOG(2) << __func__ << ": Register NTP SR component";

  const auto* value = local_pref_->Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);

  DCHECK(IsValidSuperReferralComponentInfo(*value));
  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      *value->FindStringKey(kPublicKey),
      *value->FindStringKey(kComponentID),
      base::StringPrintf("NTP Super Referral (%s)",
                         value->FindStringKey(kThemeName)->c_str()),
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr(),
                          true));
}

void NTPBackgroundImagesService::DownloadSuperReferralMappingTable() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(GetSuperReferralMappingTableURL());
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

void NTPBackgroundImagesService::OnGetMappingTableData(
    std::unique_ptr<std::string> json_string) {
  if (!json_string) {
    DVLOG(2) << __func__ << ": Failed to fetch mapping table.";
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
    DVLOG(2) << __func__ << ": This is super referral.";
    local_pref_->Set(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                     *value);
    RegisterSuperReferralComponent();
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
  if (super_referral && sr_images_data_ && sr_images_data_->IsValid())
    return sr_images_data_.get();

  if (!super_referral && si_images_data_ && si_images_data_->IsValid())
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
                     top_sites_favicon_cache_dir_, is_super_referral),
      base::BindOnce(&NTPBackgroundImagesService::OnGetComponentJsonData,
                     weak_factory_.GetWeakPtr(),
                     is_super_referral));
}

void NTPBackgroundImagesService::OnGetComponentJsonData(
    bool is_super_referral,
    const std::string& json_string) {
  if (is_super_referral) {
    sr_images_data_.reset(
        new NTPBackgroundImagesData(json_string,
                                    sr_installed_dir_,
                                    top_sites_favicon_cache_dir_));
    if (local_pref_->FindPreference(
            prefs::kNewTabPageCachedSuperReferralFaviconList)->
                IsDefaultValue() &&
        sr_images_data_->IsValid()) {
      // This is done only once because super referral will have same top sites
      // list forever.
      CacheTopSitesFaviconList();
    }
  } else {
    si_images_data_.reset(new NTPBackgroundImagesData(json_string,
                                                      si_installed_dir_,
                                                      base::FilePath()));
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
}

void NTPBackgroundImagesService::CacheTopSitesFaviconList() {
  DCHECK(sr_images_data_->IsValid());
  DCHECK(local_pref_->FindPreference(
            prefs::kNewTabPageCachedSuperReferralFaviconList)->
                IsDefaultValue());
  base::Value list(base::Value::Type::LIST);

  for (const auto& top_site : sr_images_data_->top_sites) {
    const std::string file_path = top_sites_favicon_cache_dir_.Append(
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
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo))
    return kDemoSuperReferralCode;
  return local_pref_->GetString(kReferralPromoCode);
}

void NTPBackgroundImagesService::InitializeWebUIDataSource(
    content::WebUIDataSource* html_source) {
  std::string theme_name;
  // theme name from component manifest and company name from mapping table
  // are same string.
  const auto* value = local_pref_->Get(
        prefs::kNewTabPageCachedSuperReferralComponentInfo);
  if (base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper) &&
      IsValidSuperReferralComponentInfo(*value)) {
    theme_name = *value->FindStringKey(kThemeName);
  }
  html_source->AddString("superReferralThemeName", theme_name);
}

bool NTPBackgroundImagesService::IsSuperReferral() const {
  const auto* value = local_pref_->Get(
        prefs::kNewTabPageCachedSuperReferralComponentInfo);
  return
      base::FeatureList::IsEnabled(features::kBraveNTPSuperReferralWallpaper) &&
      IsValidSuperReferralComponentInfo(*value);
}

}  // namespace ntp_background_images
