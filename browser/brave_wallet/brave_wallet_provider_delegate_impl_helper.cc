/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"

#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/notreached.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/brave_pages.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/web_contents.h"

namespace {

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] base::OnceCallback<void()>
    g_new_setup_needed_callback_for_testing;
// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] base::OnceCallback<void(std::string_view coin_name)>
    g_account_creation_callback_for_testing;

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
  Browser* browser =
      web_contents ? chrome::FindBrowserWithTab(web_contents) : nullptr;

  if (browser) {
    brave::ShowBraveWalletOnboarding(browser);
  } else if (g_new_setup_needed_callback_for_testing) {
    std::move(g_new_setup_needed_callback_for_testing).Run();
  }
}

void ShowAccountCreation(content::WebContents* web_contents,
                         brave_wallet::mojom::CoinType coin_type) {
  Browser* browser =
      web_contents ? chrome::FindBrowserWithTab(web_contents) : nullptr;

  auto it = kAccountCreationCoinName.find(coin_type);
  if (kAccountCreationCoinName.find(coin_type) ==
      kAccountCreationCoinName.end()) {
    return;
  }

  if (browser) {
    brave::ShowBraveWalletAccountCreation(browser, it->second);
  } else if (g_account_creation_callback_for_testing) {
    std::move(g_account_creation_callback_for_testing).Run(it->second);
  }
}

void WalletInteractionDetected(content::WebContents* web_contents) {}

// Desktop uses a panel to show all notifications instead of a dialog
// on Android for permissions
bool IsWeb3NotificationAllowed() {
  return true;
}

void SetCallbackForNewSetupNeededForTesting(
    base::OnceCallback<void()> callback) {
  g_new_setup_needed_callback_for_testing = std::move(callback);
}

void SetCallbackForAccountCreationForTesting(
    base::OnceCallback<void(std::string_view)> callback) {
  g_account_creation_callback_for_testing = std::move(callback);
}

}  // namespace brave_wallet
