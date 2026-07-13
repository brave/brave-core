/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_SUITE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_SUITE_H_

#include "brave/components/brave_shields/core/browser/brave_shields_test_utils.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/brave_wallet/brave_wallet_service_delegate_base.h"
#endif

#define ChromeTestSuite ChromeTestSuite_ChromiumImpl
#include <chrome/test/base/chrome_test_suite.h>  // IWYU pragma: export
#undef ChromeTestSuite

class ChromeTestSuite : public ChromeTestSuite_ChromiumImpl {
 public:
  ChromeTestSuite(int argc, char** argv);
  ~ChromeTestSuite() override;

 protected:
  // base::TestSuite overrides:
  void Initialize() override;

 private:
  // Use stable farbling both in Brave and Chromium tests.
  brave_shields::ScopedStableFarblingTokensForTesting
      scoped_stable_farbling_tokens_{1, base::Token()};
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // We don't want to lock wallet by timeout in tests. It may cause intermittent
  // CI failures.
  base::AutoReset<bool> scoped_disable_wallet_autolock_{
      brave_wallet::BraveWalletServiceDelegateBase::
          GetScopedDisableAutolockForTesting()};
#endif
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_SUITE_H_
