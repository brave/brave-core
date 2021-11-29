/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { TransactionInfo } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { HardwareVendor } from '../api/getKeyringsByType'
import { GetAccountsHardwareOperationResult, HardwareOperationResult, SignHardwareMessageOperationResult, SignHardwareTransactionOperationResult } from '../hardware_operations'

export abstract class HardwareKeyring {
  abstract getAccounts (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult>
  abstract type (): HardwareVendor
  abstract isUnlocked (): boolean
  abstract unlock (): Promise<HardwareOperationResult>
}

export abstract class TrezorKeyring extends HardwareKeyring {
  abstract signTransaction (path: string, txInfo: TransactionInfo, chainId: string): Promise<SignHardwareTransactionOperationResult>
  abstract signPersonalMessage (path: string, message: string): Promise<SignHardwareMessageOperationResult>
}

export abstract class LedgerKeyring extends HardwareKeyring {
  abstract signPersonalMessage (path: string, address: string, message: string): Promise<SignHardwareMessageOperationResult>
  abstract signTransaction (path: string, rawTxHex: string): Promise<SignHardwareTransactionOperationResult>
}
