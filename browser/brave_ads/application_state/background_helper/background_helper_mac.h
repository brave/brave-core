/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_MAC_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_MAC_H_

#include <memory>

#include "brave/components/brave_ads/browser/application_state/background_helper.h"

namespace brave_ads {

class BackgroundHelperMac : public BackgroundHelper {
 public:
  BackgroundHelperMac(const BackgroundHelperMac&) = delete;
  BackgroundHelperMac& operator=(const BackgroundHelperMac&) = delete;

  BackgroundHelperMac(BackgroundHelperMac&&) noexcept = delete;
  BackgroundHelperMac& operator=(BackgroundHelperMac&&) noexcept = delete;

  ~BackgroundHelperMac() override;

 protected:
  friend class BackgroundHelperHolder;

  BackgroundHelperMac();

 private:
  // BackgroundHelper:
  bool IsForeground() const override;

  class BackgroundHelperDelegate;
  std::unique_ptr<BackgroundHelperDelegate> delegate_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_MAC_H_
