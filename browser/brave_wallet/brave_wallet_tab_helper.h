/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "url/gurl.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));

namespace views {
class Widget;
}  // namespace views

namespace content {
class BrowserContext;
struct GlobalRenderFrameHostId;
class WebContents;

}  // namespace content

namespace brave_wallet {

class WalletBubbleManagerDelegate;

class BraveWalletTabHelper
    : public content::WebContentsUserData<BraveWalletTabHelper> {
 public:
  explicit BraveWalletTabHelper(content::WebContents* web_contents);
  ~BraveWalletTabHelper() override;

  static void BindEthereumProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<mojom::EthereumProvider> receiver);

  static void BindSolanaProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<mojom::SolanaProvider> receiver);

  static void BindCardanoProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<mojom::CardanoProvider> receiver);

  static void BindPolkadotProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<mojom::PolkadotProvider> receiver);

  // TODO(https://github.com/brave/brave-browser/issues/55073): split this tab
  // helper.
#if !BUILDFLAG(IS_ANDROID)
  void ShowBubble();
  void ShowApproveWalletBubble();
  void CloseBubble();
  bool IsShowingBubble();
  bool IsBubbleClosedForTesting();
  content::WebContents* GetBubbleWebContentsForTesting();
  views::Widget* GetBubbleWidgetForTesting();
  void SetShowBubbleCallbackForTesting(base::OnceClosure callback) {
    show_bubble_callback_for_testing_ = std::move(callback);
  }
  void SetSkipDelegateForTesting(bool skip) {
    skip_delegate_for_testing_ = skip;
  }
#endif

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletTabHelperUnitTest, GetApproveBubbleURL);
  friend class content::WebContentsUserData<BraveWalletTabHelper>;

#if !BUILDFLAG(IS_ANDROID)
  void ShowBubbleImpl(GURL url);
  GURL GetBubbleURL();
  base::OnceClosure show_bubble_callback_for_testing_;
  bool is_showing_bubble_for_testing_ = false;
  bool skip_delegate_for_testing_ = false;
  GURL GetApproveBubbleURL();
  std::unique_ptr<WalletBubbleManagerDelegate> wallet_bubble_manager_delegate_;
#endif

  mojo::UniqueReceiverSet<mojom::EthereumProvider> ethereum_provider_receivers_;
  mojo::UniqueReceiverSet<mojom::SolanaProvider> solana_provider_receivers_;
  mojo::UniqueReceiverSet<mojom::CardanoProvider> cardano_provider_receivers_;
  mojo::UniqueReceiverSet<mojom::PolkadotProvider> polkadot_provider_receivers_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
