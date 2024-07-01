// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletPageState } from '../../constants/types'

type State = Omit<WalletPageState, 'wallet'>

// safe selectors (primitive return types only)
export const hasInitialized = ({ page }: State) => page.hasInitialized
export const isFetchingNFTMetadata = ({ page }: State) =>
  page.isFetchingNFTMetadata
export const mnemonic = ({ page }: State) => page.mnemonic
export const hasMnemonic = ({ page }: State) => !!page.mnemonic
export const setupStillInProgress = ({ page }: State) =>
  page.setupStillInProgress
export const showRecoveryPhrase = ({ page }: State) => page.showRecoveryPhrase
export const walletTermsAcknowledged = ({ page }: State) =>
  page.walletTermsAcknowledged

// unsafe selectors (will cause re-render if not strictly equal "===") (objects
// and lists)
export const nftMetadata = ({ page }: State) => page.nftMetadata
export const nftMetadataError = ({ page }: State) => page.nftMetadataError
