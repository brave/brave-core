/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_EXTENDED_INFO_HANDLER_H_
#define BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_EXTENDED_INFO_HANDLER_H_

#include <string>

#include "components/sessions/content/extended_info_handler.h"

namespace content {
class NavigationEntry;
}  // namespace content

namespace speedreader {

// This class is meant to persist data to a content::NavigationEntry so that
// distilled pages will be recognized on a restored session.
class SpeedreaderExtendedInfoHandler : public sessions::ExtendedInfoHandler {
 public:
  // Register the extended info handler.
  // Calling this more than once will cause a crash.
  static void Register();

  static void PersistSpeedreaderMode(content::NavigationEntry* entry);
  static void PersistReaderMode(content::NavigationEntry* entry);
  static void ClearPersistedData(content::NavigationEntry* entry);

  static bool IsCachedSpeedreaderMode(content::NavigationEntry* entry);
  static bool IsCachedReaderMode(content::NavigationEntry* entry);

  SpeedreaderExtendedInfoHandler(const SpeedreaderExtendedInfoHandler&) =
      delete;
  SpeedreaderExtendedInfoHandler& operator=(
      const SpeedreaderExtendedInfoHandler&) = delete;

  SpeedreaderExtendedInfoHandler() = default;
  ~SpeedreaderExtendedInfoHandler() override = default;

  std::string GetExtendedInfo(content::NavigationEntry* entry) const override;
  void RestoreExtendedInfo(const std::string& info,
                           content::NavigationEntry* entry) override;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_EXTENDED_INFO_HANDLER_H_
