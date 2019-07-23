/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/common/extensions/extension_constants.h"
#endif

using BraveLocationBarModelDelegateTest = testing::Test;

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
TEST_F(BraveLocationBarModelDelegateTest, ResolvesEthereumRemoteClient) {
  GURL url(ethereum_remote_client_base_url);
  base::string16 formatted_url = base::UTF8ToUTF16(url.spec());
  BraveLocationBarModelDelegate::FormattedStringFromURL(url, &formatted_url);
  ASSERT_STREQ(base::UTF16ToASCII(formatted_url).c_str(), "brave://wallet");
}
#endif

TEST_F(BraveLocationBarModelDelegateTest, ResolvesChromeSchemeToBrave) {
  GURL url("chrome://sync/");
  base::string16 formatted_url = base::UTF8ToUTF16(url.spec());
  BraveLocationBarModelDelegate::FormattedStringFromURL(url, &formatted_url);
  ASSERT_STREQ(base::UTF16ToASCII(formatted_url).c_str(), "brave://sync/");
}
