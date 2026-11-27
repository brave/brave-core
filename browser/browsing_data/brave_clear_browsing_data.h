/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_
#define BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_

#include <cstdint>

#include "build/build_config.h"

namespace content {
class BrowsingDataRemover;
}
class Profile;
class BraveClearDataOnExitTest;

namespace content {

class BraveClearBrowsingData {
 public:
  BraveClearBrowsingData(const BraveClearBrowsingData&) = delete;
  BraveClearBrowsingData& operator=(const BraveClearBrowsingData&) = delete;

  // Clears browsing data for all loaded non-off-the-record profiles.
  // Profile's *OnExit preferences determine what gets cleared.
  // Note: this method will wait until browsing data has been cleared.
  static void ClearOnExit();

#if !BUILDFLAG(IS_ANDROID)
  // Makes sure a profile whose *OnExit preferences select data is cleared
  // before it unloads. Such a profile stays loaded until its last window
  // closes and its data is cleared, including while the application quits.
  // It unloads then only if nothing else uses it; otherwise ClearOnExit()
  // clears it again at exit. ClearOnExit() skips profiles released this way.
  // Does nothing when features::kDestroyProfileOnBrowserClose is off, because
  // profiles then stay loaded until exit.
  static void StartClearingOnProfileClose();

  // Stops the clearing that StartClearingOnProfileClose() started. Call it
  // after ClearOnExit(), before profiles are torn down.
  static void StopClearingOnProfileClose();
#endif  // !BUILDFLAG(IS_ANDROID)

  static bool IsClearOnExitEnabledForAnyType(Profile* profile);

  // Used for testing only.
  struct OnExitTestingCallback {
    // Called from ClearOnExit right before the call to BrowsingDataRemover
    // to remove data.
    virtual void BeforeClearOnExitRemoveData(
        content::BrowsingDataRemover* remover,
        uint64_t remove_mask,
        uint64_t origin_mask) = 0;
  };

 protected:
  friend class ::BraveClearDataOnExitTest;

  // Used for testing only.
  static void SetOnExitTestingCallback(OnExitTestingCallback* callback);

 private:
  static OnExitTestingCallback* on_exit_testing_callback_;
};

}  // namespace content

#endif  // BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_
