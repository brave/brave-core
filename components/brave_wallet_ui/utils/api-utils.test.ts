import { GetTokenParam, GetFlattenedAccountBalances } from './api-utils'
import { mockNetworks } from '../stories/mock-data/mock-networks'
import { AccountAssetOptions } from '../options/asset-options'
import { mockAccount } from '../common/constants/mocks'

describe('Check token param', () => {
  test('Value should return contract address', () => {
    expect(GetTokenParam(mockNetworks[0], AccountAssetOptions[1].asset)).toEqual('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
  })
  test('Value should return symbol', () => {
    expect(GetTokenParam(mockNetworks[1], AccountAssetOptions[1].asset)).toEqual('bat')
  })
})

const mockAccounts = [
  {
    ...mockAccount,
    tokens: [{ ...AccountAssetOptions[0], assetBalance: '0x350082652bcfb2e' }, AccountAssetOptions[1]]
  },
  {
    ...mockAccount,
    tokens: [{ ...AccountAssetOptions[0], assetBalance: '0x11e40b7737cd000' }, AccountAssetOptions[1]]
  }
]

const expectedResult = [
  {
    balance: 0.31930000000000003,
    token: AccountAssetOptions[0].asset
  },
  {
    balance: 0,
    token: AccountAssetOptions[1].asset
  }
]

describe('Check Flattened Account Balances', () => {
  test('Value should return an Empty Array', () => {
    expect(GetFlattenedAccountBalances([])).toEqual([])
  })
  test('Value should return a Flattened Account Balance List', () => {
    expect(GetFlattenedAccountBalances(mockAccounts)).toEqual(expectedResult)
  })
})
