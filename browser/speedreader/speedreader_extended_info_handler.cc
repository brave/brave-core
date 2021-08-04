/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_extended_info_handler.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "content/public/browser/navigation_entry.h"

namespace {

// This is the key we register in the extended info map. We also use it for the
// navigation entry user data.
constexpr char kSpeedreaderKey[] = "speedreader";

constexpr char kPageSavedReaderMode[] = "reader-mode";
constexpr char kPageSavedSpeedreaderMode[] = "speedreader-mode";

struct SpeedreaderNavigationData : public base::SupportsUserData::Data {
  explicit SpeedreaderNavigationData(const std::string& value) : value(value) {}
  std::string value;
};

}  // namespace

namespace speedreader {

// static
void SpeedreaderExtendedInfoHandler::Register() {
  auto* handler = new SpeedreaderExtendedInfoHandler();
  sessions::ContentSerializedNavigationDriver::GetInstance()
      ->RegisterExtendedInfoHandler(kSpeedreaderKey, base::WrapUnique(handler));
}

// static
void SpeedreaderExtendedInfoHandler::PersistSpeedreaderMode(
    content::NavigationEntry* entry) {
  entry->SetUserData(
      kSpeedreaderKey,
      std::make_unique<SpeedreaderNavigationData>(kPageSavedSpeedreaderMode));
}

// static
void SpeedreaderExtendedInfoHandler::PersistReaderMode(
    content::NavigationEntry* entry) {
  entry->SetUserData(
      kSpeedreaderKey,
      std::make_unique<SpeedreaderNavigationData>(kPageSavedReaderMode));
}

// static
void SpeedreaderExtendedInfoHandler::ClearPersistedData(
    content::NavigationEntry* entry) {
  entry->RemoveUserData(kSpeedreaderKey);
}

// static
bool SpeedreaderExtendedInfoHandler::IsCachedSpeedreaderMode(
    content::NavigationEntry* entry) {
  auto* data = static_cast<SpeedreaderNavigationData*>(
      entry->GetUserData(kSpeedreaderKey));
  if (!data) {
    return false;
  }
  return data->value == kPageSavedSpeedreaderMode;
}

// static
bool SpeedreaderExtendedInfoHandler::IsCachedReaderMode(
    content::NavigationEntry* entry) {
  auto* data = static_cast<SpeedreaderNavigationData*>(
      entry->GetUserData(kSpeedreaderKey));
  if (!data) {
    return false;
  }
  return data->value == kPageSavedReaderMode;
}

std::string SpeedreaderExtendedInfoHandler::GetExtendedInfo(
    content::NavigationEntry* entry) const {
  auto* data = static_cast<SpeedreaderNavigationData*>(
      entry->GetUserData(kSpeedreaderKey));
  return data ? data->value : std::string();
}

void SpeedreaderExtendedInfoHandler::RestoreExtendedInfo(
    const std::string& info,
    content::NavigationEntry* entry) {
  entry->SetUserData(kSpeedreaderKey,
                     std::make_unique<SpeedreaderNavigationData>(info));
}

}  // namespace speedreader
