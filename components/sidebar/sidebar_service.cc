/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/sidebar_service.h"

#include <algorithm>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/sidebar/features.h"
#include "brave/components/sidebar/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace sidebar {

namespace {

constexpr char kSidebarItemURLKey[] = "url";
constexpr char kSidebarItemTypeKey[] = "type";
constexpr char kSidebarItemTitleKey[] = "title";
constexpr char kSidebarItemOpenInPanelKey[] = "open_in_panel";

std::vector<SidebarItem> GetDefaultSidebarItems() {
  // TODO(simonhong): Get titles by observing webcontents.
  std::vector<SidebarItem> items;
  items.push_back(SidebarItem::Create(
      GURL("https://together.brave.com/"),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_BRAVE_TOGETHER_ITEM_TITLE),
      SidebarItem::Type::kTypeBuiltIn, true));
  items.push_back(SidebarItem::Create(
      GURL("chrome://wallet/"),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_WALLET_ITEM_TITLE),
      SidebarItem::Type::kTypeBuiltIn, false));
  items.push_back(SidebarItem::Create(
      GURL("chrome://bookmarks/"),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_BOOKMARKS_ITEM_TITLE),
      SidebarItem::Type::kTypeBuiltIn, true));
  items.push_back(SidebarItem::Create(
      GURL("chrome://history/"),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_HISTORY_ITEM_TITLE),
      SidebarItem::Type::kTypeBuiltIn, true));
  return items;
}

}  // namespace

// static
void SidebarService::RegisterPrefs(PrefRegistrySimple* registry) {
  if (!base::FeatureList::IsEnabled(kSidebarFeature))
    return;

  registry->RegisterListPref(kSidebarItems);
  registry->RegisterIntegerPref(kSidebarShowOption, kShowAlways);
}

SidebarService::SidebarService(PrefService* prefs) : prefs_(prefs) {
  DCHECK(prefs_);
  LoadSidebarItems();

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kSidebarShowOption,
      base::BindRepeating(&SidebarService::OnPreferenceChanged,
                          base::Unretained(this)));
}

SidebarService::~SidebarService() = default;

void SidebarService::AddItem(const SidebarItem& item) {
  items_.push_back(item);

  for (Observer& obs : observers_) {
    // Index starts at zero.
    obs.OnItemAdded(item, items_.size() - 1);
  }

  UpdateSidebarItemsToPrefStore();
}

void SidebarService::RemoveItemAt(int index) {
  const SidebarItem removed_item = items_[index];

  for (Observer& obs : observers_)
    obs.OnWillRemoveItem(removed_item, index);

  items_.erase(items_.begin() + index);
  for (Observer& obs : observers_)
    obs.OnItemRemoved(removed_item, index);

  UpdateSidebarItemsToPrefStore();
}

void SidebarService::UpdateSidebarItemsToPrefStore() {
  ListPrefUpdate update(prefs_, kSidebarItems);
  update->ClearList();

  for (const auto& item : items_) {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(kSidebarItemURLKey, item.url.spec());
    dict.SetStringKey(kSidebarItemTitleKey, base::UTF16ToUTF8(item.title));
    dict.SetIntKey(kSidebarItemTypeKey, static_cast<int>(item.type));
    dict.SetBoolKey(kSidebarItemOpenInPanelKey, item.open_in_panel);
    update->Append(std::move(dict));
  }
}

void SidebarService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SidebarService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::vector<SidebarItem> SidebarService::GetNotAddedDefaultSidebarItems()
    const {
  auto default_items = GetDefaultSidebarItems();
  const auto added_default_items = GetDefaultSidebarItemsFromCurrentItems();
  for (const auto& added_item : added_default_items) {
    auto iter = std::find_if(default_items.begin(), default_items.end(),
                             [added_item](const auto& default_item) {
                               return default_item.url == added_item.url;
                             });
    default_items.erase(iter);
  }
  return default_items;
}

std::vector<SidebarItem>
SidebarService::GetDefaultSidebarItemsFromCurrentItems() const {
  std::vector<SidebarItem> items;
  std::copy_if(items_.begin(), items_.end(), std::back_inserter(items),
               [](const auto& item) { return IsBuiltInType(item); });
  return items;
}

int SidebarService::GetSidebarShowOption() const {
  return prefs_->GetInteger(kSidebarShowOption);
}

void SidebarService::SetSidebarShowOption(int show_options) {
  prefs_->SetInteger(kSidebarShowOption, show_options);
}

void SidebarService::LoadSidebarItems() {
  auto* preference = prefs_->FindPreference(kSidebarItems);
  if (preference->IsDefaultValue()) {
    items_ = GetDefaultSidebarItems();
    return;
  }

  const auto& items = preference->GetValue()->GetList();
  for (const auto& item : items) {
    std::string url;
    if (const auto* value = item.FindStringKey(kSidebarItemURLKey)) {
      url = *value;
    } else {
      continue;
    }

    SidebarItem::Type type;
    if (const auto value = item.FindIntKey(kSidebarItemTypeKey)) {
      type = static_cast<SidebarItem::Type>(*value);
    } else {
      continue;
    }

    bool open_in_panel;
    if (const auto value = item.FindBoolKey(kSidebarItemOpenInPanelKey)) {
      open_in_panel = *value;
    } else {
      continue;
    }

    // Title can be updated later.
    std::string title;
    if (const auto* value = item.FindStringKey(kSidebarItemTitleKey)) {
      title = *value;
    }

    items_.push_back(SidebarItem::Create(GURL(url), base::UTF8ToUTF16(title),
                                         type, open_in_panel));
  }
}

void SidebarService::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == kSidebarShowOption) {
    for (Observer& obs : observers_)
      obs.OnShowSidebarOptionChanged(GetSidebarShowOption());
    return;
  }
}

}  // namespace sidebar
