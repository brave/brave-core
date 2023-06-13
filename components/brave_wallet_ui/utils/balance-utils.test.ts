// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getBalance, getPercentAmount } from './balance-utils'

// mocks
import { mockAccount, mockERC20Token } from '../common/constants/mocks'
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

describe('getPercentAmount', () => {
  it.each([
    ['25% of 1 ETH', '1000000000000000000', 0.25, '0.25'],
    ['50% of 1 ETH', '1000000000000000000', 0.5, '0.5'],
    ['75% of 1 ETH', '1000000000000000000', 0.75, '0.75'],
    ['100% of 1 ETH', '1000000000000000000', 1, '1'],
    [
      '100% of 1.000000000000000001 ETH',
      '1000000000000000001',
      1,
      '1.000000000000000001'
    ], // 1.000000000000000001 ---(100%)---> 1.000000000000000001
    ['100% of 50.297939 ETH', '50297939000000000000', 1, '50.297939'], // 50.297939 ---(100%)---> 50.297939
    ['25% of 0.0001 ETH', '100000000000000', 0.25, '0.000025'] // 0.0001 ---(25%)---> 0.000025
  ])(
    'should compute %s correctly',
    (_, balance: string, percent, expected: string) => {
      const account = {
        ...mockAccount,
        tokenBalanceRegistry: {
          [mockERC20Token.contractAddress.toLowerCase()]: balance
        }
      }

      expect(getPercentAmount(mockERC20Token, account, percent)).toBe(expected)
    }
  )
})
