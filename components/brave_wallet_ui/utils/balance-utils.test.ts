// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getBalance } from './balance-utils'

// mocks
import { mockAccount } from '../common/constants/mocks'
import { WalletAccountType } from '../constants/types'
import { mockBasicAttentionToken, mockBinanceCoinErc20Token } from '../stories/mock-data/mock-asset-options'

const accountWithBalances: WalletAccountType = {
  ...mockAccount,
  tokenBalanceRegistry: {
    [mockBasicAttentionToken.contractAddress.toLowerCase()]: '123'
  }
}

describe('getBalance', () => {
  it('gets a balance of a token for a given account', () => {
    expect(getBalance(accountWithBalances, mockBasicAttentionToken)).toBe('123')
  })

  it('returns an empty string if a balance of a token for a given account was not found', () => {
    expect(getBalance(accountWithBalances, mockBinanceCoinErc20Token)).toBe('')
  })
})
