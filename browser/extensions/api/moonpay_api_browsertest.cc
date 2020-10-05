/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/extensions/api/moonpay_api.h"
#include "brave/common/brave_paths.h"
#include "brave/components/moonpay/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/api_test_utils.h"
#include "extensions/common/extension_builder.h"

// npm run test -- brave_browser_tests --filter=MoonpayAPIBrowserTest.*

using extensions::api::MoonpayOnBuyBitcoinDotComCryptoFunction;
using extensions::api::MoonpayOnInteractionBitcoinDotComFunction;
using extensions::api::MoonpayGetBitcoinDotComInteractionsFunction;
using extension_function_test_utils::RunFunctionAndReturnSingleResult;
using extension_function_test_utils::RunFunction;
using extension_function_test_utils::ToDictionary;
using extensions::api_test_utils::RunFunctionFlags;

class MoonpayAPIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    extension_ = extensions::ExtensionBuilder("Test").Build();

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  }

  scoped_refptr<const extensions::Extension> extension() {
    return extension_;
  }

  Profile* profile() { return browser()->profile(); }

 private:
  scoped_refptr<const extensions::Extension> extension_;
};

IN_PROC_BROWSER_TEST_F(MoonpayAPIBrowserTest,
                       MoonpayOnBuyBitcoinDotComCryptoFunctionTest) {
  scoped_refptr<MoonpayOnBuyBitcoinDotComCryptoFunction> buy_function(
      new MoonpayOnBuyBitcoinDotComCryptoFunction());
  buy_function->set_extension(extension().get());

  // kMoonpayHasBoughtBitcoinDotComCrypto should initially be false
  ASSERT_FALSE(profile()->GetPrefs()->GetBoolean(
      kMoonpayHasBoughtBitcoinDotComCrypto));
  // kMoonpayHasInteractedBitcoinDotCom should initially be false
  ASSERT_FALSE(profile()->GetPrefs()->GetBoolean(
      kMoonpayHasInteractedBitcoinDotCom));

  // Call moonpay.onBuyBitcoinDotComCrypto
  RunFunction(buy_function.get(), std::string("[]"), browser(),
      RunFunctionFlags());

  // kMoonpayHasBoughtBitcoinDotComCrypto should be true
  ASSERT_TRUE(profile()->GetPrefs()->GetBoolean(
      kMoonpayHasBoughtBitcoinDotComCrypto));
  // kMoonpayHasBoughtBitcoinDotComCrypto should also be true
  ASSERT_TRUE(profile()->GetPrefs()->GetBoolean(
      kMoonpayHasInteractedBitcoinDotCom));
}

IN_PROC_BROWSER_TEST_F(MoonpayAPIBrowserTest,
                       MoonpayOnInteractionBitcoinDotComFunction) {
  scoped_refptr<MoonpayOnInteractionBitcoinDotComFunction> interact_function(
      new MoonpayOnInteractionBitcoinDotComFunction());
  interact_function->set_extension(extension().get());

  // kMoonpayHasInteractedBitcoinDotCom should initially be false
  ASSERT_FALSE(profile()->GetPrefs()->GetBoolean(
      kMoonpayHasInteractedBitcoinDotCom));

  // Call moonpay.onInteractionBitcoinDotCom
  RunFunction(interact_function.get(), std::string("[]"), browser(),
      RunFunctionFlags());

  // kMoonpayHasBoughtBitcoinDotComCrypto should be true
  ASSERT_TRUE(profile()->GetPrefs()->GetBoolean(
      kMoonpayHasInteractedBitcoinDotCom));
}

IN_PROC_BROWSER_TEST_F(MoonpayAPIBrowserTest,
                       MoonpayGetBitcoinDotComInteractionsFunctionNone) {
  scoped_refptr<MoonpayGetBitcoinDotComInteractionsFunction> get_function(
      new MoonpayGetBitcoinDotComInteractionsFunction());
  get_function->set_extension(extension().get());

  std::unique_ptr<base::DictionaryValue> result;
  result.reset(ToDictionary(
      RunFunctionAndReturnSingleResult(get_function.get(),
                                       std::string("[]"),
                                       browser())));
  bool interacted;
  bool bought_crypto;
  result->GetBoolean("interacted", &interacted);
  result->GetBoolean("boughtCrypto", &bought_crypto);
  EXPECT_EQ(interacted, false);
  EXPECT_EQ(bought_crypto, false);
}

IN_PROC_BROWSER_TEST_F(MoonpayAPIBrowserTest,
                       MoonpayGetBitcoinDotComInteractionsFunctionOne) {
  scoped_refptr<MoonpayOnInteractionBitcoinDotComFunction> interact_function(
      new MoonpayOnInteractionBitcoinDotComFunction());
  interact_function->set_extension(extension().get());

  // Set an interaction
  RunFunction(interact_function.get(), std::string("[]"), browser(),
      RunFunctionFlags());

  scoped_refptr<MoonpayGetBitcoinDotComInteractionsFunction> get_function(
      new MoonpayGetBitcoinDotComInteractionsFunction());
  get_function->set_extension(extension().get());

  std::unique_ptr<base::DictionaryValue> result;
  result.reset(ToDictionary(
      RunFunctionAndReturnSingleResult(get_function.get(),
                                       std::string("[]"),
                                       browser())));
  bool interacted;
  bool bought_crypto;
  result->GetBoolean("interacted", &interacted);
  result->GetBoolean("boughtCrypto", &bought_crypto);
  EXPECT_EQ(interacted, true);
  EXPECT_EQ(bought_crypto, false);
}

IN_PROC_BROWSER_TEST_F(MoonpayAPIBrowserTest,
                       MoonpayGetBitcoinDotComInteractionsFunctionBoth) {
  scoped_refptr<MoonpayOnBuyBitcoinDotComCryptoFunction> buy_function(
      new MoonpayOnBuyBitcoinDotComCryptoFunction());
  buy_function->set_extension(extension().get());

  // Mark a buy interaction
  RunFunction(buy_function.get(), std::string("[]"), browser(),
      RunFunctionFlags());

  scoped_refptr<MoonpayGetBitcoinDotComInteractionsFunction> get_function(
      new MoonpayGetBitcoinDotComInteractionsFunction());
  get_function->set_extension(extension().get());

  std::unique_ptr<base::DictionaryValue> result;
  result.reset(ToDictionary(
      RunFunctionAndReturnSingleResult(get_function.get(),
                                       std::string("[]"),
                                       browser())));
  bool interacted;
  bool bought_crypto;
  result->GetBoolean("interacted", &interacted);
  result->GetBoolean("boughtCrypto", &bought_crypto);
  EXPECT_EQ(interacted, true);
  EXPECT_EQ(bought_crypto, true);
}
