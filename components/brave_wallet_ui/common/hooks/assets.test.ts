import { renderHook, act } from '@testing-library/react-hooks'
import {
  mockAccount,
  mockAssetPrices
} from '../constants/mocks'
import { AccountAssetOptions } from '../../options/asset-options'
import useAssets from './assets'
import { AccountAssetOptionType } from '../../constants/types'

const mockAccounts = [
  {
    ...mockAccount,
    tokens: [{ ...AccountAssetOptions[0], assetBalance: '238699740940532500' }, AccountAssetOptions[1]]
  },
  {
    ...mockAccount,
    tokens: [{ ...AccountAssetOptions[0], assetBalance: '0' }, AccountAssetOptions[1]]
  }
]

const mockVisibleList = [
  AccountAssetOptions[0].asset,
  AccountAssetOptions[1].asset
]

const expectedResult = [{ asset: AccountAssetOptions[0].asset, assetBalance: '238699740940532500' }] as AccountAssetOptionType[]

const getBuyAssets = async () => {
  return await mockVisibleList
}

describe('useAssets hook', () => {
  it('Selected account has balances, should return expectedResult', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccounts[0], mockVisibleList, mockVisibleList, mockAssetPrices, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual(expectedResult)
  })
  it('Selected account has 0 balances, should return an empty array', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccounts[1], mockVisibleList, mockVisibleList, mockAssetPrices, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })
  it('Selected account tokens is undefined, should return an empty array', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccount, mockVisibleList, mockVisibleList, mockAssetPrices, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })
})
