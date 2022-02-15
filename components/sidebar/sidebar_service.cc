/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/sidebar_service.h"

#include <algorithm>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/sidebar/constants.h"
#include "brave/components/sidebar/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace sidebar {

namespace {

SidebarItem GetBuiltInItemForType(SidebarItem::BuiltInItemType type) {
  switch (type) {
    case SidebarItem::BuiltInItemType::kBraveTalk:
      return SidebarItem::Create(
          GURL(kBraveTalkURL),
          l10n_util::GetStringUTF16(IDS_SIDEBAR_BRAVE_TALK_ITEM_TITLE),
          SidebarItem::Type::kTypeBuiltIn,
          SidebarItem::BuiltInItemType::kBraveTalk, false);
    case SidebarItem::BuiltInItemType::kWallet:
      return SidebarItem::Create(
          GURL("chrome://wallet/"),
          l10n_util::GetStringUTF16(IDS_SIDEBAR_WALLET_ITEM_TITLE),
          SidebarItem::Type::kTypeBuiltIn,
          SidebarItem::BuiltInItemType::kWallet, false);
    case SidebarItem::BuiltInItemType::kBookmarks:
      return SidebarItem::Create(
          GURL(kSidebarBookmarksURL),
          l10n_util::GetStringUTF16(IDS_SIDEBAR_BOOKMARKS_ITEM_TITLE),
          SidebarItem::Type::kTypeBuiltIn,
          SidebarItem::BuiltInItemType::kBookmarks, true);
    case SidebarItem::BuiltInItemType::kHistory:
      return SidebarItem::Create(
          GURL("chrome://history/"),
          l10n_util::GetStringUTF16(IDS_SIDEBAR_HISTORY_ITEM_TITLE),
          SidebarItem::Type::kTypeBuiltIn,
          SidebarItem::BuiltInItemType::kHistory, true);
    default:
      NOTREACHED();
  }
  return SidebarItem();
}

SidebarItem::BuiltInItemType GetBuiltInItemTypeForURL(const std::string& url) {
  if (url == "https://together.brave.com/" || url == "https://talk.brave.com/")
    return SidebarItem::BuiltInItemType::kBraveTalk;

  if (url == "chrome://wallet/")
    return SidebarItem::BuiltInItemType::kWallet;

  if (url == kSidebarBookmarksURL || url == "chrome://bookmarks/")
    return SidebarItem::BuiltInItemType::kBookmarks;

  if (url == "chrome://history/")
    return SidebarItem::BuiltInItemType::kHistory;

  NOTREACHED();
  return SidebarItem::BuiltInItemType::kNone;
}

SidebarItem GetBuiltInItemForURL(const std::string& url) {
  return GetBuiltInItemForType(GetBuiltInItemTypeForURL(url));
}

std::vector<SidebarItem> GetDefaultSidebarItems() {
  std::vector<SidebarItem> items;
  items.push_back(
      GetBuiltInItemForType(SidebarItem::BuiltInItemType::kBraveTalk));
  items.push_back(GetBuiltInItemForType(SidebarItem::BuiltInItemType::kWallet));
  items.push_back(
      GetBuiltInItemForType(SidebarItem::BuiltInItemType::kBookmarks));
  return items;
}

}  // namespace

// static
void SidebarService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kSidebarItems);
  registry->RegisterIntegerPref(
      kSidebarShowOption, static_cast<int>(ShowSidebarOption::kShowAlways));
  registry->RegisterIntegerPref(kSidebarItemAddedFeedbackBubbleShowCount, 0);
}

SidebarService::SidebarService(PrefService* prefs) : prefs_(prefs) {
  DCHECK(prefs_);
  LoadSidebarItems();

  MigrateSidebarShowOptions();

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kSidebarShowOption,
      base::BindRepeating(&SidebarService::OnPreferenceChanged,
                          base::Unretained(this)));
}

SidebarService::~SidebarService() = default;

void SidebarService::MigrateSidebarShowOptions() {
  auto option =
      static_cast<ShowSidebarOption>(prefs_->GetInteger(kSidebarShowOption));
  // Show on click is deprecated. Treat it as show on mouse over.
  if (option == ShowSidebarOption::kShowOnClick) {
    option = ShowSidebarOption::kShowOnMouseOver;
    prefs_->SetInteger(kSidebarShowOption,
                       static_cast<int>(ShowSidebarOption::kShowOnMouseOver));
  }
}

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

void SidebarService::MoveItem(int from, int to) {
  DCHECK(items_.size() > static_cast<size_t>(from) &&
         items_.size() > static_cast<size_t>(to) && from >= 0 && to >= 0);

  if (from == to)
    return;

  const SidebarItem item = items_[from];
  items_.erase(items_.begin() + from);
  items_.insert(items_.begin() + to, item);

  for (Observer& obs : observers_)
    obs.OnItemMoved(item, from, to);

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
    dict.SetIntKey(kSidebarItemBuiltInItemTypeKey,
                   static_cast<int>(item.built_in_item_type));
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

std::vector<SidebarItem> SidebarService::GetHiddenDefaultSidebarItems() const {
  auto default_items = GetDefaultSidebarItems();
  const auto added_default_items = GetDefaultSidebarItemsFromCurrentItems();
  for (const auto& added_item : added_default_items) {
    auto iter = std::find_if(default_items.begin(), default_items.end(),
                             [added_item](const auto& default_item) {
                               return default_item.built_in_item_type ==
                                      added_item.built_in_item_type;
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

SidebarService::ShowSidebarOption SidebarService::GetSidebarShowOption() const {
  return static_cast<ShowSidebarOption>(prefs_->GetInteger(kSidebarShowOption));
}

void SidebarService::SetSidebarShowOption(ShowSidebarOption show_options) {
  DCHECK_NE(ShowSidebarOption::kShowOnClick, show_options);
  prefs_->SetInteger(kSidebarShowOption, static_cast<int>(show_options));
}

void SidebarService::LoadSidebarItems() {
  auto* preference = prefs_->FindPreference(kSidebarItems);
  if (preference->IsDefaultValue()) {
    items_ = GetDefaultSidebarItems();
    return;
  }

  const auto& items = preference->GetValue()->GetListDeprecated();
  for (const auto& item : items) {
    SidebarItem::Type type;
    if (const auto value = item.FindIntKey(kSidebarItemTypeKey)) {
      type = static_cast<SidebarItem::Type>(*value);
    } else {
      continue;
    }

    std::string url;
    if (const auto* value = item.FindStringKey(kSidebarItemURLKey)) {
      url = *value;
    } else {
      continue;
    }

    // Always use latest properties for built-in type item.
    if (type == SidebarItem::Type::kTypeBuiltIn) {
      SidebarItem built_in_item;
      if (const auto value = item.FindIntKey(kSidebarItemBuiltInItemTypeKey)) {
        built_in_item = GetBuiltInItemForType(
            static_cast<SidebarItem::BuiltInItemType>(*value));
      } else {
        // Fallback when built-in item type key is not existed.
        built_in_item = GetBuiltInItemForURL(url);
      }
      // Remove blocked item from existing users data.
      if (!IsBlockedBuiltInItem(built_in_item))
        items_.push_back(built_in_item);
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

    DCHECK(type != SidebarItem::Type::kTypeBuiltIn);
    items_.push_back(SidebarItem::Create(
        GURL(url), base::UTF8ToUTF16(title), type,
        SidebarItem::BuiltInItemType::kNone, open_in_panel));
  }
}

// For now, only builtin history item is blocked.
bool SidebarService::IsBlockedBuiltInItem(const SidebarItem& item) const {
  if (!IsBuiltInType(item))
    return false;
  return item.built_in_item_type == SidebarItem::BuiltInItemType::kHistory;
}

void SidebarService::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == kSidebarShowOption) {
    for (Observer& obs : observers_)
      obs.OnShowSidebarOptionChanged(GetSidebarShowOption());
    return;
  }
}

}  // namespace sidebar
