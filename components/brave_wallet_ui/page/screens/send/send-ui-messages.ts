// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  AddressMessageInfoIds,
  AddressMessageInfo
} from '../../../constants/types'

export const ENSOffchainLookupMessage: AddressMessageInfo = {
  title: 'braveWalletEnsOffChainLookupTitle',
  description: 'braveWalletEnsOffChainLookupDescription',
  url: 'https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup',
  id: AddressMessageInfoIds.ensOffchainLookupWarning,
  type: 'info'
}

export const HasNoDomainAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletNotDomain',
  id: AddressMessageInfoIds.hasNoDomainAddress,
  type: 'error'
}

export const FailedChecksumMessage: AddressMessageInfo = {
  title: 'braveWalletFailedChecksumTitle',
  description: 'braveWalletFailedChecksumDescription',
  type: 'error',
  id: AddressMessageInfoIds.invalidChecksumError
}

export const MissingChecksumMessage: AddressMessageInfo = {
  title: 'braveWalletFailedChecksumTitle',
  description: 'braveWalletFailedChecksumDescription',
  type: 'warning',
  id: AddressMessageInfoIds.missingChecksumWarning
}

export const FEVMAddressConvertionMessage: AddressMessageInfo = {
  title: 'braveWalletFEVMAddressTranslationTitle',
  description: 'braveWalletFEVMAddressTranslationDescription',
  url: 'https://docs.filecoin.io/smart-contracts/filecoin-evm-runtime/address-types/',
  type: 'warning',
  id: AddressMessageInfoIds.FEVMTranslationWarning
}

export const InvalidAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletNotValidAddress',
  type: 'error',
  id: AddressMessageInfoIds.invalidAddressError
}

export const InvalidUnifiedAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletInvalidZcashUnifiedRecipientAddress',
  type: 'error',
  id: AddressMessageInfoIds.invalidUnifiedAddressError
}

export const SameAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletSameAddressError',
  type: 'error',
  id: AddressMessageInfoIds.sameAddressError
}

export const ContractAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletContractAddressError',
  type: 'error',
  id: AddressMessageInfoIds.contractAddressError
}

export const InvalidDomainExtensionMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletInvalidDomainExtension',
  type: 'error',
  id: AddressMessageInfoIds.invalidDomainExtension
}

export const AddressValidationMessages = [
  ENSOffchainLookupMessage,
  HasNoDomainAddressMessage,
  FailedChecksumMessage,
  MissingChecksumMessage,
  FEVMAddressConvertionMessage,
  InvalidAddressMessage,
  InvalidUnifiedAddressMessage,
  SameAddressMessage,
  ContractAddressMessage,
  InvalidDomainExtensionMessage
]
