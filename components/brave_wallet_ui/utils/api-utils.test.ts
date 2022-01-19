import { GetTokenParam, GetFlattenedAccountBalances } from './api-utils'
import { mockNetworks } from '../stories/mock-data/mock-networks'
import { AccountAssetOptions } from '../options/asset-options'
import { mockAccount } from '../common/constants/mocks'

describe('Check token param', () => {
  test('Value should return contract address', () => {
    expect(GetTokenParam(mockNetworks[0], AccountAssetOptions[1])).toEqual('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
  })

  test('Value should return symbol', () => {
    expect(GetTokenParam(mockNetworks[1], AccountAssetOptions[1])).toEqual('bat')
  })

  test('Value should return coingeckoId', () => {
    expect(GetTokenParam(mockNetworks[1], {
      ...AccountAssetOptions[1],
      coingeckoId: 'mockCoingeckoId'
    })).toEqual('mockCoingeckoId')
  })
})

const mockUserVisibleTokenOptions = [
  AccountAssetOptions[0],
  AccountAssetOptions[1]
]
const mockAccounts = [
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [AccountAssetOptions[0].contractAddress.toLowerCase()]: '238699740940532526',
      [AccountAssetOptions[1].contractAddress.toLowerCase()]: '0'
    }
  },
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [AccountAssetOptions[0].contractAddress.toLowerCase()]: '80573000000000000',
      [AccountAssetOptions[1].contractAddress.toLowerCase()]: '0'
    }
  }
]

const expectedResult = [
  {
    balance: 319272740940532500,
    token: AccountAssetOptions[0]
  },
  {
    balance: 0,
    token: AccountAssetOptions[1]
  }
]

describe('Check Flattened Account Balances', () => {
  test('Value should return an Empty Array', () => {
    expect(GetFlattenedAccountBalances([], mockUserVisibleTokenOptions)).toEqual([])
  })
  test('Value should return a Flattened Account Balance List', () => {
    expect(GetFlattenedAccountBalances(mockAccounts, mockUserVisibleTokenOptions)).toEqual(expectedResult)
  })
})
