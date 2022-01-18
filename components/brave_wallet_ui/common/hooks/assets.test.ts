import { renderHook, act } from '@testing-library/react-hooks'
import {
  mockAccount,
  mockAssetPrices,
  mockNetwork
} from '../constants/mocks'
import { AccountAssetOptions } from '../../options/asset-options'
import useAssets from './assets'
import { WalletAccountType } from '../../constants/types'

const mockAccounts = [
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [AccountAssetOptions[0].contractAddress.toLowerCase()]: '238699740940532500',
      [AccountAssetOptions[1].contractAddress.toLowerCase()]: '0'
    }
  } as WalletAccountType,
  {
    ...mockAccount,
    balance: '',
    tokenBalanceRegistry: {
      [AccountAssetOptions[0].contractAddress.toLowerCase()]: '0',
      [AccountAssetOptions[1].contractAddress.toLowerCase()]: '0'
    }
  } as WalletAccountType
]

const mockVisibleList = [
  AccountAssetOptions[0],
  AccountAssetOptions[1]
]

const expectedResult = [AccountAssetOptions[0]]

const getBuyAssets = async () => {
  return await mockVisibleList
}

describe('useAssets hook', () => {
  it('Selected account has balances, should return expectedResult', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccounts[0], mockNetwork, mockVisibleList, mockVisibleList, mockAssetPrices, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual(expectedResult)
  })

  it('Selected account has 0 balances, should return an empty array', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccounts[1], mockNetwork, mockVisibleList, mockVisibleList, mockAssetPrices, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })

  it('should return empty array for panelUserAssetList if visible assets is empty', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccount, mockNetwork, mockVisibleList, [], mockAssetPrices, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })
})
