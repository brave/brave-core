/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'
import { HardwareVendor } from '../api/hardware_keyrings'
import {
  FilecoinNetwork,
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareOperationResult
} from './types'

export abstract class HardwareKeyring {
  abstract coin (): BraveWallet.CoinType
  abstract type (): HardwareVendor
  abstract unlock (): Promise<HardwareOperationResult>
}

export abstract class TrezorKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult>
  abstract signTransaction (path: string, txInfo: SerializableTransactionInfo, chainId: string): Promise<SignHardwareOperationResult>
  abstract signPersonalMessage (path: string, message: string): Promise<SignHardwareOperationResult>
  abstract signEip712Message (path: string, domainSeparatorHex: string, hashStructMessageHex: string): Promise<SignHardwareOperationResult>
}

export abstract class LedgerEthereumKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult>
  abstract signPersonalMessage (path: string, address: string, message: string): Promise<SignHardwareOperationResult>
  abstract signTransaction (path: string, rawTxHex: string): Promise<SignHardwareOperationResult>
  abstract signEip712Message (path: string, domainSeparatorHex: string, hashStructMessageHex: string): Promise<SignHardwareOperationResult>
}

export abstract class LedgerFilecoinKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, network: FilecoinNetwork): Promise<GetAccountsHardwareOperationResult>
  abstract signTransaction (message: string): Promise<SignHardwareOperationResult>
}

export abstract class LedgerSolanaKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult>
  abstract signTransaction (path: string, rawTxBytes: Buffer): Promise<SignHardwareOperationResult>
}
