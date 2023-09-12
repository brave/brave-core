// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, PageState } from '../../constants/types'
import { mockNFTMetadata } from './mock-nft-metadata'
import { mockedMnemonic } from './user-accounts'

export const mockPageState: PageState = {
  isFetchingNFTMetadata: false,
  nftMetadata: mockNFTMetadata[0],
  nftMetadataError: 'Something went wrong when fetching NFT details. Please try again later.',
  hasInitialized: false,
  importWalletError: {
    hasError: false
  },
  invalidMnemonic: false,
  isCryptoWalletsInitialized: false,
  isMetaMaskInitialized: false,
  selectedAsset: undefined,
  pinStatusOverview: undefined,
  selectedCoinMarket: undefined,
  selectedTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  setupStillInProgress: false,
  showIsRestoring: false,
  showRecoveryPhrase: false,
  isImportWalletsCheckComplete: false,
  importWalletAttempts: 0,
  walletTermsAcknowledged: false,
  mnemonic: mockedMnemonic
    .replace('tomato', 'FIRST')
    .replace('velvet', 'THIRD')
    .concat(` ${mockedMnemonic} LAST`),
  enablingAutoPin: false,
  isAutoPinEnabled: false
}
