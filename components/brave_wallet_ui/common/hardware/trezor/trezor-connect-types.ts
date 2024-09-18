// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/**
 *  Note these types are copied from trezor-connect so that an import is
 *  not needed outside of the untrusted-frame. If you require new additional
 *  types, visit https://github.com/trezor/connect/blob/develop/src/ts/types/networks/ethereum.d.ts
 *  where they can be copied from.
 */

export interface Unsuccessful {
  success: false
  payload: { error: string; code?: string }
}

export interface Success<T> {
  success: true
  payload: T
}

export interface EthereumSignTransaction {
  path: string | number[]
  transaction: EthereumTransaction | EthereumTransactionEIP1559
}

export interface EthereumTransaction {
  to: string
  value: string
  gasPrice: string
  gasLimit: string
  maxFeePerGas?: typeof undefined
  maxPriorityFeePerGas?: typeof undefined
  nonce: string
  data?: string
  chainId: number
  txType?: number
}

export type EthereumTransactionEIP1559 = {
  to: string
  value: string
  gasLimit: string
  gasPrice?: typeof undefined
  nonce: string
  data?: string
  chainId: number
  maxFeePerGas: string
  maxPriorityFeePerGas: string
  accessList?: EthereumAccessList[]
}

export type EthereumAccessList = {
  address: string
  storageKeys: string[]
}

export interface CommonParams {
  device?: {
    path: string
    state?: string
    instance?: number
  }
  useEmptyPassphrase?: boolean
  allowSeedlessDevice?: boolean
  keepSession?: boolean
  skipFinalReload?: boolean
  useCardanoDerivation?: boolean
}

export interface EthereumSignMessage {
  path: string | number[]
  message: string
  hex?: boolean
}

export interface EthereumSignTypedHash {
  path: string | number[]
  domain_separator_hash: string
  message_hash: string
  metamask_v4_compat: boolean
  data: EthereumSignTypedDataMessage<EthereumSignTypedDataTypes>
}

export interface EthereumSignTypedDataTypeProperty {
  name: string
  type: string
}

export interface EthereumSignTypedDataTypes {
  EIP712Domain: EthereumSignTypedDataTypeProperty[]
  [additionalProperties: string]: EthereumSignTypedDataTypeProperty[]
}

export interface EthereumSignTypedDataMessage<
  T extends EthereumSignTypedDataTypes
> {
  types: T
  primaryType: keyof T
  domain: {
    name?: string
    version?: string
    chainId?: number | bigint
    verifyingContract?: string
    salt?: ArrayBuffer
  }
  message: { [fieldName: string]: any }
}
