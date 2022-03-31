import { AccountAssetOptions } from '../../../options/asset-options'
import { BraveWallet } from '../../../constants/types'

let mockedAllowance = '1000000000000000000' // 1 unit
let mockedIsSwapSupported = true

export const getERC20Allowance = (
  _fromAddress: string,
  _selectedAccountAddress: string,
  allowanceTarget: string
) => new Promise<string>((resolve) => {
  resolve(allowanceTarget || mockedAllowance)
})

export const setERC20Allowance = (newAllowance: string) => {
  mockedAllowance = newAllowance
}

export const getIsSwapSupported = (network: BraveWallet.NetworkInfo) => new Promise<boolean>((resolve) => {
  resolve(mockedIsSwapSupported)
})

let mockBuyAssetList = [
  AccountAssetOptions[0],
  AccountAssetOptions[1]
]

export const getBuyAssets = () => new Promise<typeof AccountAssetOptions>((resolve) => {
  resolve(mockBuyAssetList)
})

export const setMockedBuyAssets = (newList: typeof AccountAssetOptions) => {
  mockBuyAssetList = newList
}
