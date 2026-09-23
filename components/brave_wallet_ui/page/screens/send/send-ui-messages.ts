// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  AddressMessageInfoIds,
  AddressMessageInfo,
} from '../../../constants/types'

export const ENSOffchainLookupMessage: AddressMessageInfo = {
  title: S.BRAVE_WALLET_ENS_OFF_CHAIN_LOOKUP_TITLE,
  description: S.BRAVE_WALLET_ENS_OFF_CHAIN_LOOKUP_DESCRIPTION,
  url: 'https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup',
  id: AddressMessageInfoIds.ensOffchainLookupWarning,
  type: 'info',
}

export const HasNoDomainAddressMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_NOT_DOMAIN,
  id: AddressMessageInfoIds.hasNoDomainAddress,
  type: 'error',
}

export const FailedChecksumMessage: AddressMessageInfo = {
  title: S.BRAVE_WALLET_INVALID_CHECKSUM_TITLE,
  description: S.BRAVE_WALLET_INVALID_CHECKSUM_DESCRIPTION,
  type: 'error',
  id: AddressMessageInfoIds.invalidChecksumError,
}

export const MissingChecksumMessage: AddressMessageInfo = {
  title: S.BRAVE_WALLET_MISSING_CHECKSUM_TITLE,
  description: S.BRAVE_WALLET_MISSING_CHECKSUM_DESCRIPTION,
  type: 'warning',
  id: AddressMessageInfoIds.missingChecksumWarning,
}

export const FEVMAddressConversionMessage: AddressMessageInfo = {
  title: S.BRAVE_WALLET_FEVM_ADDRESS_TRANSLATION_TITLE,
  description: S.BRAVE_WALLET_FEVM_ADDRESS_TRANSLATION_DESCRIPTION,
  url: 'https://docs.filecoin.io/smart-contracts/filecoin-evm-runtime/address-types/',
  type: 'warning',
  id: AddressMessageInfoIds.FEVMTranslationWarning,
}

export const InvalidAddressMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_NOT_VALID_ADDRESS,
  type: 'error',
  id: AddressMessageInfoIds.invalidAddressError,
}

export const SameAddressMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_SAME_ADDRESS_ERROR,
  type: 'error',
  id: AddressMessageInfoIds.sameAddressError,
}

export const ContractAddressMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_CONTRACT_ADDRESS_ERROR,
  type: 'error',
  id: AddressMessageInfoIds.contractAddressError,
}

export const InvalidDomainExtensionMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_INVALID_DOMAIN_EXTENSION,
  type: 'error',
  id: AddressMessageInfoIds.invalidDomainExtension,
}

// ZCash
export const ZCashInvalidTransparentAddressErrorMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_ZCASH_INVALID_TRANSPARENT_ADDRESS,
  type: 'error',
  id: AddressMessageInfoIds.zcashInvalidTransparentAddressError,
}

export const ZCashInvalidUnifiedAddressErrorMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_ZCASH_INVALID_UNIFIED_ADDRESS,
  type: 'error',
  id: AddressMessageInfoIds.zcashInvalidUnifiedAddressError,
}

export const ZCashInvalidUnifiedAddressMissingOrchardPartErrorMessage: AddressMessageInfo =
  {
    title: '',
    description:
      S.BRAVE_WALLET_ZCASH_INVALID_UNIFIED_ADDRESS_MISSING_ORCHARD_PART,
    type: 'error',
    id: AddressMessageInfoIds.zcashInvalidUnifiedAddressMissingOrchardPartError,
  }

export const ZCashInvalidUnifiedAddressMissingTransparentPartErrorMessage: AddressMessageInfo =
  {
    title: '',
    description:
      S.BRAVE_WALLET_ZCASH_INVALID_UNIFIED_ADDRESS_MISSING_TRANSPARENT_PART,
    type: 'error',

    id: AddressMessageInfoIds.zcashInvalidUnifiedAddressMissingTransparentPartError,
  }

export const ZCashInvalidAddressNetworkMismatchErrorMessage: AddressMessageInfo =
  {
    title: '',
    description: S.BRAVE_WALLET_ZCASH_INVALID_ADDRESS_NETWORK_MISMATCH,
    type: 'error',
    id: AddressMessageInfoIds.zcashInvalidAddressNetworkMismatchError,
  }

// Polkadot
export const PolkadotInvalidPrefixErrorMessage: AddressMessageInfo = {
  title: '',
  description: S.BRAVE_WALLET_POLKADOT_INVALID_PREFIX,
  type: 'error',
  id: AddressMessageInfoIds.polkadotInvalidPrefixError,
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
  PolkadotInvalidPrefixErrorMessage,
]
