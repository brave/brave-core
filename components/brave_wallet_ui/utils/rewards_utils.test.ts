// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Mock Data
import { mockAccount, mockNetwork } from '../common/constants/mocks'
import { mockBasicAttentionToken } from '../stories/mock-data/mock-asset-options'

// Utils
import {
  getIsRewardsAccount,
  getNormalizedExternalRewardsWallet,
  getNormalizedExternalRewardsNetwork,
  getIsRewardsNetwork,
  getIsRewardsToken,
  getRewardsBATToken,
  getRewardsAccountName,
  getRewardsProviderBackground,
  getRewardsProviderIcon,
  getRewardsProviderName,
  getRewardsTokenDescription
} from './rewards_utils'

const upholdRewardsAccount = getNormalizedExternalRewardsWallet('uphold')

describe('getIsRewardsAccount', () => {
  it('Should return true', () => {
    expect(getIsRewardsAccount(upholdRewardsAccount?.accountId)).toBeTruthy()
  })
  it('Should return false', () => {
    expect(getIsRewardsAccount(mockAccount.accountId)).toBeFalsy()
  })
})

describe('getNormalizedExternalRewardsWallet', () => {
  it('Should return uphold as uniqueKey', () => {
    expect(getNormalizedExternalRewardsWallet('uphold')?.accountId.uniqueKey)
      .toEqual('uphold')
  })
  it('Should return undefined', () => {
    expect(getNormalizedExternalRewardsWallet(undefined))
      .toEqual(undefined)
  })
})

const upholdRewardsNetwork = getNormalizedExternalRewardsNetwork('uphold')

describe('getIsRewardsNetwork', () => {
  it('Should return true', () => {
    expect(getIsRewardsNetwork(upholdRewardsNetwork)).toBeTruthy()
  })
  it('Should return false', () => {
    expect(getIsRewardsNetwork(mockNetwork)).toBeFalsy()
  })
})

describe('getNormalizedExternalRewardsNetwork', () => {
  it('Should return uphold as chainId', () => {
    expect(getNormalizedExternalRewardsNetwork('uphold')?.chainId)
      .toEqual('uphold')
  })
  it('Should return undefined', () => {
    expect(getNormalizedExternalRewardsNetwork(undefined))
      .toEqual(undefined)
  })
})

const upholdRewardsToken = getRewardsBATToken('uphold')

describe('getIsRewardsToken', () => {
  it('Should return true', () => {
    expect(getIsRewardsToken(upholdRewardsToken)).toBeTruthy()
  })
  it('Should return false', () => {
    expect(getIsRewardsToken(mockBasicAttentionToken)).toBeFalsy()
  })
})

describe('getRewardsBATToken', () => {
  it('Should return uphold as chainId', () => {
    expect(getRewardsBATToken('uphold')?.chainId)
      .toEqual('uphold')
  })
  it('Should return undefined', () => {
    expect(getRewardsBATToken(undefined))
      .toEqual(undefined)
  })
})

describe('getRewardsAccountName', () => {
  it('Should return Uphold account as name', () => {
    expect(getRewardsAccountName('uphold'))
      .toEqual('braveWalletRewardsAccount')
  })
  it('Should return empty string', () => {
    expect(getRewardsAccountName(undefined))
      .toEqual('')
  })
})

describe('getRewardsProviderBackground', () => {
  it('Should return Uphold color', () => {
    expect(getRewardsProviderBackground('uphold'))
      .toEqual('rgb(73, 204, 104)')
  })
  it('Should return empty string', () => {
    expect(getRewardsProviderBackground(null))
      .toEqual('')
  })
})

describe('getRewardsProviderIcon', () => {
  it('Should return empty string', () => {
    expect(getRewardsProviderIcon(null))
      .toEqual('')
  })
})

describe('getRewardsProviderName', () => {
  it('Should return uphold', () => {
    expect(getRewardsProviderName('uphold'))
      .toEqual('uphold')
  })
  it('Should return empty string', () => {
    expect(getRewardsProviderName(''))
      .toEqual('')
  })
})

describe('getRewardsTokenDescription', () => {
  it('Should return empty string', () => {
    expect(getRewardsTokenDescription('uphold'))
      .toEqual('braveWalletBraveRewardsDescription')
  })
  it('Should return empty string', () => {
    expect(getRewardsProviderName(''))
      .toEqual('')
  })
})
