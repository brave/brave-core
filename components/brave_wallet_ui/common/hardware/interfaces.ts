/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveWallet } from '../../constants/types'
import {
  HardwareImportScheme,
  HardwareOperationResult,
  HardwareOperationResultEthereumSignatureVRS,
  HardwareOperationResultAccounts,
  HardwareOperationResultEthereumSignatureBytes,
  HardwareOperationResultSolanaSignature,
  HardwareOperationResultFilecoinSignature,
  HardwareOperationResultBitcoinSignature
} from './types'
import { BridgeType } from './untrusted_shared_types'

export abstract class HardwareKeyring {
  abstract bridgeType(): BridgeType
  abstract getAccounts(
    from: number,
    count: number,
    scheme: HardwareImportScheme
  ): Promise<HardwareOperationResultAccounts>
  abstract unlock(): Promise<HardwareOperationResult>
}

export abstract class TrezorKeyring extends HardwareKeyring {
  abstract signTransaction(
    path: string,
    txid: string,
    ethTxData1559: BraveWallet.TxData1559,
    chainId: string
  ): Promise<HardwareOperationResultEthereumSignatureVRS>
  abstract signPersonalMessage(
    path: string,
    message: string
  ): Promise<HardwareOperationResultEthereumSignatureBytes>
  abstract signEip712Message(
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string
  ): Promise<HardwareOperationResultEthereumSignatureBytes>
}

export abstract class LedgerEthereumKeyring extends HardwareKeyring {
  abstract signTransaction(
    path: string,
    rawTxHex: string
  ): Promise<HardwareOperationResultEthereumSignatureVRS>
  abstract signPersonalMessage(
    path: string,
    address: string,
    message: string
  ): Promise<HardwareOperationResultEthereumSignatureBytes>
  abstract signEip712Message(
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string
  ): Promise<HardwareOperationResultEthereumSignatureBytes>
}

export abstract class LedgerFilecoinKeyring extends HardwareKeyring {
  abstract signTransaction(
    message: string
  ): Promise<HardwareOperationResultFilecoinSignature>
}

export abstract class LedgerBitcoinKeyring extends HardwareKeyring {
  abstract signTransaction(
    inputTransactions: Array<{
      txBytes: Buffer
      outputIndex: number
      associatedPath: string
    }>,
    outputScript: Buffer,
    changePath: string | undefined,
    lockTime: number
  ): Promise<HardwareOperationResultBitcoinSignature>
}

export abstract class LedgerSolanaKeyring extends HardwareKeyring {
  abstract signTransaction(
    path: string,
    rawTxBytes: Buffer
  ): Promise<HardwareOperationResultSolanaSignature>
}
