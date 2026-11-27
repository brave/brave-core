// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, HardwareWalletOperation } from '$wallet/constants/types'
import { mockAccount, mockEthAccount } from '$wallet/common/constants/mocks'
import { mockSignMessageRequest } from '$wallet/stories/mock-data/mock-eth-requests'
import { createPanelSlice } from '$wallet/panel/slices/panel.slice'
import {
  hardwareOperationMatchesSignMessage,
  hardwareOperationMatchesTransaction,
} from './hardware_wallet_retry'

const hardwareAccountId: BraveWallet.AccountId = {
  ...mockEthAccount.accountId,
  kind: BraveWallet.AccountKind.kHardware,
  uniqueKey: 'hw-eth-account',
}

const signOperation: HardwareWalletOperation = {
  type: 'signMessage',
  id: 7,
  accountUniqueKey: hardwareAccountId.uniqueKey,
}

const hardwareSignRequest: BraveWallet.SignMessageRequest = {
  ...mockSignMessageRequest,
  id: 7,
  accountId: hardwareAccountId,
}

const hotTransaction = {
  id: 'hot-tx',
  fromAccountId: mockAccount.accountId,
}

const hardwareTransaction = {
  id: 'hw-tx',
  fromAccountId: hardwareAccountId,
}

describe('hardware wallet retry binding', () => {
  it('matches only the hardware sign request that opened the panel', () => {
    expect(
      hardwareOperationMatchesSignMessage(signOperation, hardwareSignRequest),
    ).toBe(true)
    expect(
      hardwareOperationMatchesSignMessage(signOperation, {
        ...hardwareSignRequest,
        id: 8,
      }),
    ).toBe(false)
    expect(
      hardwareOperationMatchesSignMessage(signOperation, {
        ...hardwareSignRequest,
        accountId: mockAccount.accountId,
      }),
    ).toBe(false)
    expect(
      hardwareOperationMatchesSignMessage(undefined, hardwareSignRequest),
    ).toBe(false)
  })

  it('does not match a hot-account transaction', () => {
    const transactionOperation: HardwareWalletOperation = {
      type: 'transaction',
      id: hotTransaction.id,
      accountUniqueKey: hotTransaction.fromAccountId.uniqueKey,
    }

    expect(
      hardwareOperationMatchesTransaction(transactionOperation, hotTransaction),
    ).toBe(false)
    expect(
      hardwareOperationMatchesTransaction(signOperation, hotTransaction),
    ).toBe(false)
    expect(
      hardwareOperationMatchesTransaction(
        {
          type: 'transaction',
          id: hardwareTransaction.id,
          accountUniqueKey: hardwareAccountId.uniqueKey,
        },
        hardwareTransaction,
      ),
    ).toBe(true)
    expect(
      hardwareOperationMatchesTransaction(
        {
          type: 'transaction',
          id: hardwareTransaction.id,
          accountUniqueKey: hardwareAccountId.uniqueKey,
        },
        hotTransaction,
      ),
    ).toBe(false)
  })

  it('keeps the first operation until the panel changes', () => {
    const slice = createPanelSlice()
    let state = slice.getInitialState()

    state = slice.reducer(
      state,
      slice.actions.navigateTo('connectHardwareWallet'),
    )
    state = slice.reducer(
      state,
      slice.actions.setHardwareWalletOperation(signOperation),
    )
    state = slice.reducer(
      state,
      slice.actions.setHardwareWalletOperation({
        type: 'transaction',
        id: hotTransaction.id,
        accountUniqueKey: hotTransaction.fromAccountId.uniqueKey,
      }),
    )

    expect(state.hardwareWalletOperation).toEqual(signOperation)

    state = slice.reducer(state, slice.actions.navigateTo('main'))
    expect(state.hardwareWalletOperation).toBeUndefined()
  })
})
