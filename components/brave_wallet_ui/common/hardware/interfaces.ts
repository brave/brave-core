/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import { CoinType } from '@glif/filecoin-address'
import { BraveWallet } from '../../constants/types'
import { HardwareVendor } from '../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareMessageOperationResult,
  SignHardwareTransactionOperationResult
} from './types'

export abstract class HardwareKeyring {
  abstract coin (): BraveWallet.CoinType
  abstract type (): HardwareVendor
  abstract isUnlocked (): boolean
  abstract unlock (): Promise<HardwareOperationResult>
}

export abstract class TrezorKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult>
  abstract signTransaction (path: string, txInfo: BraveWallet.TransactionInfo, chainId: string): Promise<SignHardwareTransactionOperationResult>
  abstract signPersonalMessage (path: string, message: string): Promise<SignHardwareMessageOperationResult>
}

export abstract class LedgerEthereumKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult>
  abstract signPersonalMessage (path: string, address: string, message: string): Promise<SignHardwareMessageOperationResult>
  abstract signTransaction (path: string, rawTxHex: string): Promise<SignHardwareTransactionOperationResult>
}

export abstract class LedgerFilecoinKeyring extends HardwareKeyring {
  abstract getAccounts (from: number, to: number, coinType?: CoinType): Promise<GetAccountsHardwareOperationResult>
}
