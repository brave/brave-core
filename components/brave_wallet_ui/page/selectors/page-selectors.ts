// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletPageState } from '../../constants/types'

type State = Omit<WalletPageState, 'wallet'>

// safe selectors (primitive return types only)
export const hasInitialized = ({ page }: State) => page.hasInitialized
export const importAccountError = ({ page }: State) => page.importAccountError
export const importWalletAttempts = ({ page }: State) => page.importWalletAttempts
export const invalidMnemonic = ({ page }: State) => page.invalidMnemonic
export const isCryptoWalletsInitialized = ({ page }: State) => page.isCryptoWalletsInitialized
export const isFetchingNFTMetadata = ({ page }: State) => page.isFetchingNFTMetadata
export const isFetchingPriceHistory = ({ page }: State) => page.isFetchingPriceHistory
export const isImportWalletsCheckComplete = ({ page }: State) => page.isImportWalletsCheckComplete
export const isMetaMaskInitialized = ({ page }: State) => page.isMetaMaskInitialized
export const mnemonic = ({ page }: State) => page.mnemonic
export const hasMnemonic = ({ page }: State) => !!page.mnemonic
export const selectedTimeline = ({ page }: State) => page.selectedTimeline
export const setupStillInProgress = ({ page }: State) => page.setupStillInProgress
export const showAddModal = ({ page }: State) => page.showAddModal
export const showIsRestoring = ({ page }: State) => page.showIsRestoring
export const showRecoveryPhrase = ({ page }: State) => page.showRecoveryPhrase
export const walletTermsAcknowledged = ({ page }: State) => page.walletTermsAcknowledged
export const enablingAutoPin = ({ page }: State) => page.enablingAutoPin
export const isAutoPinEnabled = ({ page }: State) => page.isAutoPinEnabled
export const isLocalIpfsNodeRunning = ({ page }: State) => page.isLocalIpfsNodeRunning

// unsafe selectors (will cause re-render if not strictly equal "===") (objects and lists)
export const importWalletError = ({ page }: State) => page.importWalletError
export const nftMetadata = ({ page }: State) => page.nftMetadata
export const nftMetadataError = ({ page }: State) => page.nftMetadataError

export const portfolioPriceHistory = ({ page }: State) => page.portfolioPriceHistory
export const selectedAsset = ({ page }: State) => page?.selectedAsset
export const pinStatusOverview = ({ page }: State) => page.pinStatusOverview
export const selectedAssetCryptoPrice = ({ page }: State) => page.selectedAssetCryptoPrice
export const selectedAssetFiatPrice = ({ page }: State) => page.selectedAssetFiatPrice
export const selectedAssetPriceHistory = ({ page }: State) => page.selectedAssetPriceHistory
export const selectedCoinMarket = ({ page }: State) => page.selectedCoinMarket
export const nftsPinningStatus = ({ page }: State) => page.nftsPinningStatus
