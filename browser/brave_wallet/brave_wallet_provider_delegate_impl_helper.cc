/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/check_is_test.h"
#include "base/containers/fixed_flat_map.h"
#include "base/notreached.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/brave_pages.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "content/public/browser/web_contents.h"

namespace {

base::OnceCallback<void()>* g_new_setup_needed_callback_for_testing = nullptr;
base::OnceCallback<void(std::string_view coin_name)>*
    g_account_creation_callback_for_testing = nullptr;

// These are names of coins used by `create-account-options.ts`. We support only
// Solana and Cardano account creation triggered by dApp.
constexpr auto kAccountCreationCoinName =
    base::MakeFixedFlatMap<brave_wallet::mojom::CoinType, std::string_view>(
        {{brave_wallet::mojom::CoinType::SOL, "Solana"},
         {brave_wallet::mojom::CoinType::ADA, "Cardano"}});
}  // namespace

namespace brave_wallet {

void ShowPanel(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents);
  if (tab_helper) {
    tab_helper->ShowBubble();
  }
}

void ShowWalletBackup() {
  NOTREACHED();
}

void UnlockWallet() {
  NOTREACHED();
}

void ShowWalletOnboarding(content::WebContents* web_contents) {
  if (web_contents) {
    BrowserWindowInterface* browser =
        GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(
            web_contents);
    if (browser) {
      brave::ShowBraveWalletOnboarding(browser);
      return;
    }
  }

  if (g_new_setup_needed_callback_for_testing) {
    CHECK_IS_TEST();
    CHECK(*g_new_setup_needed_callback_for_testing);
    std::move(*g_new_setup_needed_callback_for_testing).Run();
  }
}

void ShowAccountCreation(content::WebContents* web_contents,
                         brave_wallet::mojom::CoinType coin_type) {
  auto it = kAccountCreationCoinName.find(coin_type);
  if (kAccountCreationCoinName.find(coin_type) ==
      kAccountCreationCoinName.end()) {
    return;
  }

  if (web_contents) {
    BrowserWindowInterface* browser =
        GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(
            web_contents);
    if (browser) {
      brave::ShowBraveWalletAccountCreation(browser, it->second);
      return;
    }
  }

  if (g_account_creation_callback_for_testing) {
    CHECK_IS_TEST();
    CHECK(*g_account_creation_callback_for_testing);
    std::move(*g_account_creation_callback_for_testing).Run(it->second);
  }
}

void WalletInteractionDetected(content::WebContents* web_contents) {}

// Desktop uses a panel to show all notifications instead of a dialog
// on Android for permissions
bool IsWeb3NotificationAllowed() {
  return true;
}

base::AutoReset<base::OnceCallback<void()>*>
SetNewSetupNeededCallbackForTesting(base::OnceCallback<void()>* callback) {
  CHECK_IS_TEST();
  return base::AutoReset<base::OnceCallback<void()>*>(
      &g_new_setup_needed_callback_for_testing, callback);
}

base::AutoReset<base::OnceCallback<void(std::string_view)>*>
SetAccountCreationCallbackForTesting(
    base::OnceCallback<void(std::string_view)>* callback) {
  CHECK_IS_TEST();
  return base::AutoReset<base::OnceCallback<void(std::string_view)>*>(
      &g_account_creation_callback_for_testing, callback);
}

}  // namespace brave_wallet
