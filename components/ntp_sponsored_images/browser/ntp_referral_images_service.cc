// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_service.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/optional.h"
#include "base/task/post_task.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_component_installer.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_mapper_component_installer.h"
#include "brave/components/ntp_sponsored_images/common/pref_names.h"
#include "brave/components/ntp_sponsored_images/browser/switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ntp_sponsored_images {

namespace {

constexpr char kJsonDataFilename[] = "data.json";
constexpr char kPublicKeyPath[] = "publicKey";
constexpr char kComponentID[] = "componentID";
constexpr char kComponentName[] = "companyName";

std::string ReadJsonFile(const base::FilePath& json_path) {
  std::string contents;
  bool success = base::ReadFileToString(json_path, &contents);
  if (!success || contents.empty()) {
    DVLOG(2) << __func__
             << " : cannot read data.json file " << json_path;
  }
  return contents;
}

bool IsValidComponentInfo(const base::Value& component_info) {
  if (!component_info.FindStringKey(kPublicKeyPath))
    return false;
  if (!component_info.FindStringKey(kComponentID))
    return false;
  if (!component_info.FindStringKey(kComponentName))
    return false;

  return true;
}

bool IsDefaultReferralCode(const std::string& code) {
  return brave::BraveReferralsService::IsDefaultReferralCode(code);
}

}  // namespace

void NTPReferralImagesService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kReferralImagesServiceComponent);
}

NTPReferralImagesService::NTPReferralImagesService(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_pref)
    : cus_(cus),
      local_pref_(local_pref),
      images_data_(new NTPReferralImagesData),
      weak_factory_(this) {
  if (!cus_ || !local_pref) {
    is_super_referral_ = false;
    return;
  }

  base::FilePath forced_local_path(base::CommandLine::ForCurrentProcess()->
      GetSwitchValueNative(switches::kNTPReferralDataPathForTesting));
  if (!forced_local_path.empty()) {
    DVLOG(2)
        << "NTP Referral Image package will be loaded from local path at: "
        << forced_local_path.LossyDisplayName();
    OnReferralComponentReady(forced_local_path);
    return;
  }

  // If we already have referral component info, just register it and use data
  // from it.
  const base::Value* value =
      local_pref_->Get(prefs::kReferralImagesServiceComponent);
  if (IsValidComponentInfo(*value)) {
    RegisterReferralComponent();
    return;
  }

  // We don't need to do anything if this install has default referral code.
  if (IsDefaultReferralCode(local_pref_->GetString(kReferralPromoCode))) {
    is_super_referral_ = false;
    return;
  }

  // Register mapper component when we get referral code and it's not a default
  // referral code.
  pref_change_registrar_.Init(local_pref_);
  pref_change_registrar_.Add(kReferralPromoCode,
      base::BindRepeating(&NTPReferralImagesService::OnPreferenceChanged,
      base::Unretained(this)));
}

NTPReferralImagesService::~NTPReferralImagesService() {}

void NTPReferralImagesService::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void NTPReferralImagesService::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool NTPReferralImagesService::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

NTPReferralImagesData* NTPReferralImagesService::GetReferralImagesData() const {
  if (!is_super_referral_)
    return nullptr;

  DCHECK(images_data_);
  return images_data_.get();
}

void NTPReferralImagesService::OnPreferenceChanged() {
  if (IsDefaultReferralCode(local_pref_->GetString(kReferralPromoCode))) {
    is_super_referral_ = false;
    return;
  }

  // If this install has referral code, we should check it is super referral or
  // not by using mapper component.
  RegisterNTPReferralMapperComponent(
      cus_,
      base::BindRepeating(&NTPReferralImagesService::OnMapperComponentReady,
                          weak_factory_.GetWeakPtr()));
}

void NTPReferralImagesService::OnReferralComponentReady(
    const base::FilePath& installed_dir) {
  images_data_.reset(new NTPReferralImagesData);

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadJsonFile,
                     installed_dir.AppendASCII(kJsonDataFilename)),
      base::BindOnce(&NTPReferralImagesService::OnGetReferralJsonData,
                     weak_factory_.GetWeakPtr(),
                     installed_dir));
}

void NTPReferralImagesService::OnGetReferralJsonData(
    const base::FilePath& installed_dir,
    const std::string& json) {
  std::unique_ptr<NTPReferralImagesData> new_data(
      new NTPReferralImagesData(json, installed_dir));
  if (new_data->IsValid()) {
    images_data_ = std::move(new_data);
  } else {
    // If updated data is invalid, that means this referrer's campaign is end.
    is_super_referral_ = false;
    local_pref_->ClearPref(prefs::kReferralImagesServiceComponent);
  }

  NotifyObservers();
}

void NTPReferralImagesService::NotifyObservers() {
  for (auto& observer : observer_list_)
    observer.OnReferralImagesUpdated(GetReferralImagesData());
}

void NTPReferralImagesService::OnMapperComponentReady(
    const base::FilePath& installed_dir) {
  // If referral code is default value, we don't need to do anything.
  // Just unregister mapper component.
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadJsonFile,
                     installed_dir.AppendASCII(kJsonDataFilename)),
      base::BindOnce(&NTPReferralImagesService::OnGetMappingJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPReferralImagesService::OnGetMappingJsonData(const std::string& json) {
  base::Optional<base::Value> mapping_table_value =
      base::JSONReader::Read(json);
  if (!mapping_table_value) {
    NOTREACHED();
    is_super_referral_ = false;
    return;
  }

  const std::string referral_code  = local_pref_->GetString(kReferralPromoCode);
  if (auto* value = mapping_table_value->FindDictKey(referral_code)) {
    if (!IsValidComponentInfo(*value)) {
      NOTREACHED();
      is_super_referral_ = false;
      return;
    }

    local_pref_->Set(prefs::kReferralImagesServiceComponent, value->Clone());
    RegisterReferralComponent();
  } else {
    // This install is not for super-referral if mapping table doesn't have it
    // for current code.
    is_super_referral_ = false;
  }
}

void NTPReferralImagesService::RegisterReferralComponent() {
  const base::Value* value =
      local_pref_->Get(prefs::kReferralImagesServiceComponent);
  DCHECK(value && IsValidComponentInfo(*value));
  RegisterNTPReferralComponent(
      cus_,
      *value->FindStringKey(kPublicKeyPath),
      *value->FindStringKey(kComponentID),
      *value->FindStringKey(kComponentName),
      base::BindRepeating(&NTPReferralImagesService::OnReferralComponentReady,
                          weak_factory_.GetWeakPtr()));
}

}  // namespace ntp_sponsored_images
