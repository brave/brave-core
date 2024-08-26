/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { SerializableTransactionInfo } from '../../constants/types'
import {
  HardwareImportScheme,
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareOperationResult
} from './types'
import { BridgeType } from './untrusted_shared_types'

export abstract class HardwareKeyring {
  abstract bridgeType(): BridgeType
  abstract getAccounts(
    from: number,
    count: number,
    scheme: HardwareImportScheme
  ): Promise<GetAccountsHardwareOperationResult>
  abstract unlock(): Promise<HardwareOperationResult>
}

export abstract class TrezorKeyring extends HardwareKeyring {
  abstract signTransaction(
    path: string,
    txInfo: SerializableTransactionInfo,
    chainId: string
  ): Promise<SignHardwareOperationResult>
  abstract signPersonalMessage(
    path: string,
    message: string
  ): Promise<SignHardwareOperationResult>
  abstract signEip712Message(
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string
  ): Promise<SignHardwareOperationResult>
}

export abstract class LedgerEthereumKeyring extends HardwareKeyring {
  abstract signPersonalMessage(
    path: string,
    address: string,
    message: string
  ): Promise<SignHardwareOperationResult>
  abstract signTransaction(
    path: string,
    rawTxHex: string
  ): Promise<SignHardwareOperationResult>
  abstract signEip712Message(
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string
  ): Promise<SignHardwareOperationResult>
}

export abstract class LedgerFilecoinKeyring extends HardwareKeyring {
  abstract signTransaction(
    message: string
  ): Promise<SignHardwareOperationResult>
}

export abstract class LedgerSolanaKeyring extends HardwareKeyring {
  abstract signTransaction(
    path: string,
    rawTxBytes: Buffer
  ): Promise<SignHardwareOperationResult>
}

export abstract class LedgerBitcoinKeyring extends HardwareKeyring {}
