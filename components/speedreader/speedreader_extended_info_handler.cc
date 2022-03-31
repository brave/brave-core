/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_extended_info_handler.h"

#include <memory>
#include <utility>

#include "base/supports_user_data.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "content/public/browser/navigation_entry.h"

namespace {

// This is the key we register in the extended info map. We also use it for the
// navigation entry user data.
constexpr char kSpeedreaderKey[] = "speedreader";

constexpr char kPageSavedDistilled[] = "distilled";

struct SpeedreaderNavigationData : public base::SupportsUserData::Data {
  explicit SpeedreaderNavigationData(const std::string& value) : value(value) {}
  std::string value;
};

}  // namespace

namespace speedreader {

// static
void SpeedreaderExtendedInfoHandler::Register() {
  sessions::ContentSerializedNavigationDriver::GetInstance()
      ->RegisterExtendedInfoHandler(
          kSpeedreaderKey, std::make_unique<SpeedreaderExtendedInfoHandler>());
}

// static
void SpeedreaderExtendedInfoHandler::PersistMode(
    content::NavigationEntry* entry,
    DistillState state) {
  DCHECK(entry);
  std::unique_ptr<SpeedreaderNavigationData> data;
  switch (state) {
    case DistillState::kReaderMode:
    case DistillState::kSpeedreaderMode:
      data = std::make_unique<SpeedreaderNavigationData>(kPageSavedDistilled);
      break;
    default:
      return;
  }

  entry->SetUserData(kSpeedreaderKey, std::move(data));
}

// static
DistillState SpeedreaderExtendedInfoHandler::GetCachedMode(
    content::NavigationEntry* entry,
    SpeedreaderService* service) {
  DCHECK(entry);
  auto* data = static_cast<SpeedreaderNavigationData*>(
      entry->GetUserData(kSpeedreaderKey));
  if (!data) {
    return DistillState::kUnknown;
  }
  if (data->value == kPageSavedDistilled) {
    return service->IsEnabled() ? DistillState::kSpeedreaderMode
                                : DistillState::kReaderMode;
  } else {
    return DistillState::kUnknown;
  }
}

// static
void SpeedreaderExtendedInfoHandler::ClearPersistedData(
    content::NavigationEntry* entry) {
  entry->RemoveUserData(kSpeedreaderKey);
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
