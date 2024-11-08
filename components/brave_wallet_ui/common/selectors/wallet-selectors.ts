// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletPageState } from '../../constants/types'

type State = Omit<WalletPageState, 'page'>

// safe selectors (primitive return types only)
export const hasInitialized = ({ wallet }: State) => wallet.hasInitialized
export const isBitcoinEnabled = ({ wallet }: State) => wallet.isBitcoinEnabled
export const isBitcoinImportEnabled = ({ wallet }: State) =>
  wallet.isBitcoinImportEnabled
export const isBitcoinLedgerEnabled = ({ wallet }: State) =>
  wallet.isBitcoinLedgerEnabled
export const isZCashEnabled = ({ wallet }: State) => wallet.isZCashEnabled
export const isWalletCreated = ({ wallet }: State) => wallet.isWalletCreated
export const isWalletLocked = ({ wallet }: State) => wallet.isWalletLocked
export const passwordAttempts = ({ wallet }: State) => wallet.passwordAttempts
export const assetAutoDiscoveryCompleted = ({ wallet }: State) =>
  wallet.assetAutoDiscoveryCompleted
export const isAnkrBalancesFeatureEnabled = ({ wallet }: State) =>
  wallet.isAnkrBalancesFeatureEnabled
export const isRefreshingNetworksAndTokens = ({ wallet }: State) =>
  wallet.isRefreshingNetworksAndTokens
export const isZCashShieldedTransactionsEnabled = ({ wallet }: State) =>
  wallet.isZCashShieldedTransactionsEnabled

// unsafe selectors (will cause re-render if not strictly equal "===") (objects
// and lists)
export const allowedNewWalletAccountTypeNetworkIds = ({ wallet }: State) =>
  wallet.allowedNewWalletAccountTypeNetworkIds
