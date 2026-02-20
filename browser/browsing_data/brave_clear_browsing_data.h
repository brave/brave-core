/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_
#define BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_

#include <cstdint>

#include "build/build_config.h"

class Profile;
class BraveClearDataOnExitTest;

class BraveClearBrowsingData {
 public:
  BraveClearBrowsingData(const BraveClearBrowsingData&) = delete;
  BraveClearBrowsingData& operator=(const BraveClearBrowsingData&) = delete;

  // Clears browsing data for all loaded non-OTR profiles. The profile's *OnExit
  // preferences determine what gets cleared. This method uses a RunLoop to wait
  // until browsing data has been cleared.
  static void ClearOnShutdown();

#if !BUILDFLAG(IS_ANDROID)
  // Clears browsing data for a profile when the last browser window associated
  // with the profile is closed.
  static void ClearOnBrowserClosed(Profile* profile);
#endif  // !BUILDFLAG(IS_ANDROID)

  static bool IsClearOnExitEnabledForAnyType(Profile* profile);

  // Used for testing only.
  struct OnExitTestingCallback {
    // Called from ClearOnExit right before the call to BrowsingDataRemover
    // to remove data.
    virtual void BeforeClearOnExitRemoveData(Profile* profile,
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

#endif  // BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_
