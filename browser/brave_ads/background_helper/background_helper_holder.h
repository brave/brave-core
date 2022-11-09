/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_HOLDER_H_
#define BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_HOLDER_H_

#include <memory>

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

namespace brave_ads {

class BackgroundHelper;

class BackgroundHelperHolder final {
 public:
  BackgroundHelperHolder(const BackgroundHelperHolder&) = delete;
  BackgroundHelperHolder& operator=(const BackgroundHelperHolder&) = delete;

  BackgroundHelperHolder(BackgroundHelperHolder&& other) noexcept = delete;
  BackgroundHelperHolder& operator=(BackgroundHelperHolder&& other) noexcept =
      delete;

  static BackgroundHelperHolder* GetInstance();

  BackgroundHelper* GetBackgroundHelper();

 private:
  friend struct base::DefaultSingletonTraits<BackgroundHelperHolder>;

  BackgroundHelperHolder();

  ~BackgroundHelperHolder();

  std::unique_ptr<BackgroundHelper> background_helper_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_HOLDER_H_
