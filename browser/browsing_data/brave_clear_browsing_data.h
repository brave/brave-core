/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_
#define BRAVE_BROWSER_BROWSING_DATA_BRAVE_CLEAR_BROWSING_DATA_H_

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

  // Used for testing only.
  struct OnExitTestingCallback {
    // Called from ClearOnExit right before the call to BrowsingDataRemover
    // to remove data.
    virtual void BeforeClearOnExitRemoveData(
        content::BrowsingDataRemover* remover,
        int remove_mask,
        int origin_mask) = 0;
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
