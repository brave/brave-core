import { getTokenParam, getFlattenedAccountBalances } from './api-utils'
import { AccountAssetOptions } from '../options/asset-options'
import { mockAccount } from '../common/constants/mocks'

const ethToken = AccountAssetOptions[0]
const batToken = AccountAssetOptions[1]

describe('Check token param', () => {
  test('Value should return contract address', () => {
    expect(getTokenParam(batToken)).toEqual('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
  })

  test('Value should return symbol', () => {
    expect(getTokenParam({
      ...batToken,
      contractAddress: ''
    })).toEqual('bat')
  })

  test('Value should return coingeckoId', () => {
    expect(getTokenParam({
      ...batToken,
      coingeckoId: 'mockCoingeckoId'
    })).toEqual('mockCoingeckoId')
  })
})

const mockUserVisibleTokenOptions = [
  ethToken,
  batToken
]
const mockAccounts = [
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [ethToken.contractAddress.toLowerCase()]: '238699740940532526',
      [batToken.contractAddress.toLowerCase()]: '0'
    }
  },
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [ethToken.contractAddress.toLowerCase()]: '80573000000000000',
      [batToken.contractAddress.toLowerCase()]: '0'
    }
  }
]

const expectedResult = [
  {
    balance: 319272740940532500,
    token: ethToken
  },
  {
    balance: 0,
    token: batToken
  }
]

describe('Check Flattened Account Balances', () => {
  test('Value should return an Empty Array', () => {
    expect(getFlattenedAccountBalances([], mockUserVisibleTokenOptions)).toEqual([])
  })
  test('Value should return a Flattened Account Balance List', () => {
    expect(getFlattenedAccountBalances(mockAccounts, mockUserVisibleTokenOptions)).toEqual(expectedResult)
  })
})
