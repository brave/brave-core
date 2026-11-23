// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, HardwareWalletOperation } from '$wallet/constants/types'
import { isHardwareAccount } from '$wallet/utils/account-utils'

type RetryTransaction = {
  id: string
  fromAccountId: BraveWallet.AccountId
}

export function hardwareOperationMatchesSignMessage(
  operation: HardwareWalletOperation | undefined,
  request: BraveWallet.SignMessageRequest | undefined,
): request is BraveWallet.SignMessageRequest {
  return (
    operation?.type === 'signMessage'
    && !!request
    && request.id === operation.id
    && request.accountId.uniqueKey === operation.accountUniqueKey
    && isHardwareAccount(request.accountId)
  )
}

export function hardwareOperationMatchesTransaction(
  operation: HardwareWalletOperation | undefined,
  transaction: RetryTransaction | undefined,
): transaction is RetryTransaction {
  return (
    operation?.type === 'transaction'
    && !!transaction
    && transaction.id === operation.id
    && transaction.fromAccountId.uniqueKey === operation.accountUniqueKey
    && isHardwareAccount(transaction.fromAccountId)
  )
}
