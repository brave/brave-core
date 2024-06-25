/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_background_prefs.h"

#include <memory>
#include <utility>

#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "brave/components/constants/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {

constexpr char kTypeKey[] = "type";
constexpr char kRandomKey[] = "random";
constexpr char kSelectedValueKey[] = "selected_value";

const char* TypeToString(NTPBackgroundPrefs::Type type) {
  // See class description for details.
  switch (type) {
    case NTPBackgroundPrefs::Type::kBrave:
      return "brave";
    case NTPBackgroundPrefs::Type::kCustomImage:
      return "custom_image";
    case NTPBackgroundPrefs::Type::kColor:
      return "color";
  }
}

NTPBackgroundPrefs::Type StringToType(const std::string& type_string) {
  // See class description for details.
  if (type_string == "brave")
    return NTPBackgroundPrefs::Type::kBrave;
  if (type_string == "custom_image")
    return NTPBackgroundPrefs::Type::kCustomImage;
  if (type_string == "solid_color" || type_string == "color")
    return NTPBackgroundPrefs::Type::kColor;

  NOTREACHED_IN_MIGRATION();
  return NTPBackgroundPrefs::Type::kBrave;
}

}  // namespace

NTPBackgroundPrefs::NTPBackgroundPrefs(PrefService* service)
    : service_(service) {}

NTPBackgroundPrefs::~NTPBackgroundPrefs() = default;

// static
void NTPBackgroundPrefs::RegisterPref(
    user_prefs::PrefRegistrySyncable* registry) {
  base::Value::Dict dict;
  dict.Set(kTypeKey, TypeToString(Type::kBrave));
  dict.Set(kRandomKey, false);
  dict.Set(kSelectedValueKey, "");
  registry->RegisterDictionaryPref(kPrefName, std::move(dict));

  registry->RegisterListPref(kCustomImageListPrefName);
}

void NTPBackgroundPrefs::MigrateOldPref() {
  if (!service_->HasPrefPath(kDeprecatedPrefName))
    return;

  if (service_->GetBoolean(kDeprecatedPrefName))
    SetType(Type::kCustomImage);

  service_->ClearPref(kDeprecatedPrefName);
}

NTPBackgroundPrefs::Type NTPBackgroundPrefs::GetType() const {
  const auto* value = GetPrefValue();
  const auto* type_string = value->FindString(kTypeKey);
  return StringToType(*type_string);
}

void NTPBackgroundPrefs::SetType(Type type) {
  if (type == GetType())
    return;

  ScopedDictPrefUpdate update(service_, kPrefName);
  update->Set(kTypeKey, TypeToString(type));
}

bool NTPBackgroundPrefs::IsBraveType() const {
  return GetType() == Type::kBrave;
}

bool NTPBackgroundPrefs::IsCustomImageType() const {
  return GetType() == Type::kCustomImage;
}

bool NTPBackgroundPrefs::IsColorType() const {
  return GetType() == Type::kColor;
}

bool NTPBackgroundPrefs::ShouldUseRandomValue() const {
  const auto* value = GetPrefValue();
  auto optional_bool = value->FindBool(kRandomKey);
  return optional_bool.value_or(IsBraveType());
}

void NTPBackgroundPrefs::SetShouldUseRandomValue(bool random) {
  ScopedDictPrefUpdate update(service_, kPrefName);
  update->Set(kRandomKey, random);
}

void NTPBackgroundPrefs::SetSelectedValue(const std::string& value) {
  ScopedDictPrefUpdate update(service_, kPrefName);
  update->Set(kSelectedValueKey, value);
}

std::string NTPBackgroundPrefs::GetSelectedValue() const {
  const auto* value = GetPrefValue();
  const auto* selected_value = value->FindString(kSelectedValueKey);
  DCHECK(selected_value);

  return *selected_value;
}

void NTPBackgroundPrefs::AddCustomImageToList(const std::string& file_name) {
  ScopedListPrefUpdate update(service_,
                              NTPBackgroundPrefs::kCustomImageListPrefName);
  update->Append(file_name);
}

void NTPBackgroundPrefs::RemoveCustomImageFromList(
    const std::string& file_name) {
  ScopedListPrefUpdate update(service_,
                              NTPBackgroundPrefs::kCustomImageListPrefName);
  update->erase(base::ranges::remove(update.Get(), file_name),
                update.Get().end());
}

std::vector<std::string> NTPBackgroundPrefs::GetCustomImageList() const {
  const auto& list = service_->GetList(kCustomImageListPrefName);
  std::vector<std::string> result;
  for (const auto& item : list)
    result.push_back(item.GetString());

  return result;
}

const base::Value::Dict* NTPBackgroundPrefs::GetPrefValue() const {
  return &service_->GetDict(kPrefName);
}
