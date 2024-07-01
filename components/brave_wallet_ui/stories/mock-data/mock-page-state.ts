// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { PageState } from '../../constants/types'
import { mockNFTMetadata } from './mock-nft-metadata'
import { mockedMnemonic } from './user-accounts'

export const mockPageState: PageState = {
  isFetchingNFTMetadata: false,
  nftMetadata: mockNFTMetadata[0],
  nftMetadataError:
    'Something went wrong when fetching NFT details. Please try again later.',
  hasInitialized: false,
  setupStillInProgress: false,
  showRecoveryPhrase: false,
  walletTermsAcknowledged: false,
  mnemonic: mockedMnemonic
    .replace('tomato', 'FIRST')
    .replace('velvet', 'THIRD')
    .concat(` ${mockedMnemonic} LAST`),
  enablingAutoPin: false,
  isAutoPinEnabled: false
}
