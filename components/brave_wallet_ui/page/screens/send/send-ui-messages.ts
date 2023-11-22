// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AddressMessageInfo } from '../../../constants/types'

export const ENSOffchainLookupMessage: AddressMessageInfo = {
  title: 'braveWalletEnsOffChainLookupTitle',
  description: 'braveWalletEnsOffChainLookupDescription',
  url: 'https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup'
}

export const FailedChecksumMessage: AddressMessageInfo = {
  title: 'braveWalletFailedChecksumTitle',
  description: 'braveWalletFailedChecksumDescription'
}

export const FEVMAddressConvertionMessage: AddressMessageInfo = {
  title: 'braveWalletFEVMAddressTranslationTitle',
  description: 'braveWalletFEVMAddressTranslationDescription',
  url: 'https://docs.filecoin.io/smart-contracts/filecoin-evm-runtime/address-types/',
  type: 'warning'
}
