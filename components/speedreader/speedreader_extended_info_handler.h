/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_EXTENDED_INFO_HANDLER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_EXTENDED_INFO_HANDLER_H_

#include <string>

#include "brave/components/speedreader/speedreader_util.h"
#include "components/sessions/content/extended_info_handler.h"

namespace content {
class NavigationEntry;
}  // namespace content

namespace speedreader {

class SpeedreaderService;

// This class is meant to persist data to a content::NavigationEntry so that
// distilled pages will be recognized on a restored session.
class SpeedreaderExtendedInfoHandler : public sessions::ExtendedInfoHandler {
 public:
  // Register the extended info handler.
  // Calling this more than once will cause a crash.
  static void Register();

  // Persist the current speedreader state to NavigationEntry
  static void PersistMode(content::NavigationEntry* entry, DistillState state);

  // Retrieve cached speedreader state from NavigationEntry. Returns
  // DistillState::kUnknown if not cached.
  static DistillState GetCachedMode(content::NavigationEntry* entry,
                                    SpeedreaderService* service);

  // Clear the NavigationEntry speedreader state
  static void ClearPersistedData(content::NavigationEntry* entry);

  SpeedreaderExtendedInfoHandler() = default;
  ~SpeedreaderExtendedInfoHandler() override = default;
  SpeedreaderExtendedInfoHandler(const SpeedreaderExtendedInfoHandler&) =
      delete;
  SpeedreaderExtendedInfoHandler& operator=(
      const SpeedreaderExtendedInfoHandler&) = delete;

  // sessions::ExtendedInfoHandler:
  std::string GetExtendedInfo(content::NavigationEntry* entry) const override;
  void RestoreExtendedInfo(const std::string& info,
                           content::NavigationEntry* entry) override;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_EXTENDED_INFO_HANDLER_H_
