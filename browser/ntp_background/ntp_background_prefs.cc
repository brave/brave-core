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
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

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

  NOTREACHED();
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
  registry->RegisterDictionaryPref(kPrefName, base::Value(std::move(dict)));

  registry->RegisterListPref(kCustomImageListPrefName,
                             base::Value(base::Value::Type::LIST));
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

  prefs::ScopedDictionaryPrefUpdate update(service_, kPrefName);
  update->Set(kTypeKey, std::make_unique<base::Value>(TypeToString(type)));
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
  prefs::ScopedDictionaryPrefUpdate update(service_, kPrefName);
  update->Set(kRandomKey, std::make_unique<base::Value>(random));
}

void NTPBackgroundPrefs::SetSelectedValue(const std::string& value) {
  prefs::ScopedDictionaryPrefUpdate update(service_, kPrefName);
  update->Set(kSelectedValueKey, std::make_unique<base::Value>(value));
}

absl::variant<GURL, std::string> NTPBackgroundPrefs::GetSelectedValue() const {
  const auto* value = GetPrefValue();
  const auto* selected_value = value->FindString(kSelectedValueKey);
  DCHECK(selected_value);

  if (IsColorType() || IsCustomImageType())
    return *selected_value;

  return GURL(*selected_value);
}

void NTPBackgroundPrefs::AddCustomImageToList(const std::string& file_name) {
  ListPrefUpdate update(service_, NTPBackgroundPrefs::kCustomImageListPrefName);
  update->GetList().Append(file_name);
}

void NTPBackgroundPrefs::RemoveCustomImageFromList(
    const std::string& file_name) {
  ListPrefUpdate update(service_, NTPBackgroundPrefs::kCustomImageListPrefName);
  auto& list = update->GetList();
  list.erase(base::ranges::remove(update->GetList(), file_name), list.end());
}

std::vector<std::string> NTPBackgroundPrefs::GetCustomImageList() const {
  const auto& list = service_->GetList(kCustomImageListPrefName);
  std::vector<std::string> result;
  for (const auto& item : list)
    result.push_back(item.GetString());

  return result;
}

const base::Value::Dict* NTPBackgroundPrefs::GetPrefValue() const {
  const auto* value = service_->GetDictionary(kPrefName);
  DCHECK(value && value->is_dict());
  return value->GetIfDict();
}
