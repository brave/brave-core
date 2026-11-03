// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_wallet/wallet_bubble_host_tab.h"

WalletBubbleHostTab::WalletBubbleHostTab(
    content::WebContents* host,
    base::WeakPtr<content::WebContents> tab)
    : content::WebContentsUserData<WalletBubbleHostTab>(*host),
      tab_(std::move(tab)) {}

WalletBubbleHostTab::~WalletBubbleHostTab() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(WalletBubbleHostTab);
