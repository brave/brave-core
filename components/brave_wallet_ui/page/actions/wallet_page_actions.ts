/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PageActions } from '../reducers/page_reducer'

// We must re-export actions here until we remove all imports of this file
export const {
  agreeToWalletTerms,
  recoveryWordsAvailable,
  setIsFetchingNFTMetadata,
  updateNFTMetadata,
  updateNftMetadataError,
  walletCreated,
  walletSetupComplete
} = PageActions
