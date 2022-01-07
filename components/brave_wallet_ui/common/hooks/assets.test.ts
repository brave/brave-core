import { renderHook, act } from '@testing-library/react-hooks'
import {
  mockAccount
} from '../constants/mocks'
import { AccountAssetOptions } from '../../options/asset-options'
import useAssets from './assets'

const mockAccounts = [
  {
    ...mockAccount,
    tokens: [{ ...AccountAssetOptions[0], assetBalance: '0x350082652bcfb2e', fiatBalance: '100' }, AccountAssetOptions[1]]
  },
  {
    ...mockAccount,
    tokens: [{ ...AccountAssetOptions[0], assetBalance: '0', fiatBalance: '0' }, AccountAssetOptions[1]]
  }
]

const mockVisibleList = [
  AccountAssetOptions[0].asset,
  AccountAssetOptions[1].asset
]

const expectedResult = [{ asset: AccountAssetOptions[0].asset, assetBalance: '0.2387', fiatBalance: '100' }]

const getBuyAssets = async () => {
  return await mockVisibleList
}

describe('useAssets hook', () => {
  it('Selected account has balances, should return expectedResult', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccounts[0], mockVisibleList, mockVisibleList, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual(expectedResult)
  })
  it('Selected account has 0 balances, should return an empty array', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccounts[1], mockVisibleList, mockVisibleList, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })
  it('Selected account tokens is undefined, should return an empty array', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useAssets(mockAccounts, mockAccount, mockVisibleList, mockVisibleList, getBuyAssets))
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })
})
