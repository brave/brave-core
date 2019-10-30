/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/browser/locale_helper.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class LocaleHelperMock : public LocaleHelper {
 public:
  LocaleHelperMock();
  ~LocaleHelperMock() override;

  MOCK_CONST_METHOD0(GetLocale, const std::string());

 private:
  DISALLOW_COPY_AND_ASSIGN(LocaleHelperMock);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_MOCK_H_
