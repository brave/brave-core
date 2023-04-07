// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace chrome.braveWallet {
  const ready: () => void
  const shouldPromptForSetup: (callback: (shouldPrompt: boolean) => void) => void
  const loadUI: (callback: () => void) => void
  const isNativeWalletEnabled: (callback: (enabled: boolean) => void) => void
  const isNftPinningEnabled: (callback: (enabled: boolean) => void) => void
  const notifyWalletUnlock: () => void
  const getWeb3ProviderList: (callback: (types: string) => void) => void
  const getPinnedNftCount: (callback: (val: number) => void) => void
  const clearPinnedNft: (callback: (val: boolean) => void) => void
}
