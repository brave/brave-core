// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  AddressMessageInfoIds,
  AddressMessageInfo,
} from '../../../constants/types'

export const ENSOffchainLookupMessage: AddressMessageInfo = {
  title: 'braveWalletEnsOffChainLookupTitle',
  description: 'braveWalletEnsOffChainLookupDescription',
  url: 'https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup',
  id: AddressMessageInfoIds.ensOffchainLookupWarning,
  type: 'info',
}

export const HasNoDomainAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletNotDomain',
  id: AddressMessageInfoIds.hasNoDomainAddress,
  type: 'error',
}

export const FailedChecksumMessage: AddressMessageInfo = {
  title: 'braveWalletInvalidChecksumTitle',
  description: 'braveWalletInvalidChecksumDescription',
  type: 'error',
  id: AddressMessageInfoIds.invalidChecksumError,
}

export const MissingChecksumMessage: AddressMessageInfo = {
  title: 'braveWalletMissingChecksumTitle',
  description: 'braveWalletMissingChecksumDescription',
  type: 'warning',
  id: AddressMessageInfoIds.missingChecksumWarning,
}

export const FEVMAddressConversionMessage: AddressMessageInfo = {
  title: 'braveWalletFEVMAddressTranslationTitle',
  description: 'braveWalletFEVMAddressTranslationDescription',
  url: 'https://docs.filecoin.io/smart-contracts/filecoin-evm-runtime/address-types/',
  type: 'warning',
  id: AddressMessageInfoIds.FEVMTranslationWarning,
}

export const InvalidAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletNotValidAddress',
  type: 'error',
  id: AddressMessageInfoIds.invalidAddressError,
}

export const SameAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletSameAddressError',
  type: 'error',
  id: AddressMessageInfoIds.sameAddressError,
}

export const ContractAddressMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletContractAddressError',
  type: 'error',
  id: AddressMessageInfoIds.contractAddressError,
}

export const InvalidDomainExtensionMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletInvalidDomainExtension',
  type: 'error',
  id: AddressMessageInfoIds.invalidDomainExtension,
}

// ZCash
export const ZCashInvalidTransparentAddressErrorMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletZCashInvalidTransparentAddress',
  type: 'error',
  id: AddressMessageInfoIds.zcashInvalidTransparentAddressError,
}

export const ZCashInvalidUnifiedAddressErrorMessage: AddressMessageInfo = {
  title: '',
  description: 'braveWalletZCashInvalidUnifiedAddress',
  type: 'error',
  id: AddressMessageInfoIds.zcashInvalidUnifiedAddressError,
}

export const ZCashInvalidUnifiedAddressMissingOrchardPartErrorMessage: AddressMessageInfo =
  {
    title: '',
    description: 'braveWalletZCashInvalidUnifiedAddressMissingOrchardPart',
    type: 'error',
    id: AddressMessageInfoIds.zcashInvalidUnifiedAddressMissingOrchardPartError,
  }

export const ZCashInvalidUnifiedAddressMissingTransparentPartErrorMessage: AddressMessageInfo =
  {
    title: '',
    description: 'braveWalletZCashInvalidUnifiedAddressMissingTransparentPart',
    type: 'error',

    id: AddressMessageInfoIds.zcashInvalidUnifiedAddressMissingTransparentPartError,
  }

export const ZCashInvalidAddressNetworkMismatchErrorMessage: AddressMessageInfo =
  {
    title: '',
    description: 'braveWalletZCashInvalidAddressNetworkMismatch',
    type: 'error',
    id: AddressMessageInfoIds.zcashInvalidAddressNetworkMismatchError,
  }

export const AddressValidationMessages = [
  ENSOffchainLookupMessage,
  HasNoDomainAddressMessage,
  FailedChecksumMessage,
  MissingChecksumMessage,
  FEVMAddressConversionMessage,
  InvalidAddressMessage,
  SameAddressMessage,
  ContractAddressMessage,
  InvalidDomainExtensionMessage,
  ZCashInvalidTransparentAddressErrorMessage,
  ZCashInvalidUnifiedAddressErrorMessage,
  ZCashInvalidUnifiedAddressMissingOrchardPartErrorMessage,
  ZCashInvalidUnifiedAddressMissingTransparentPartErrorMessage,
  ZCashInvalidAddressNetworkMismatchErrorMessage,
]
