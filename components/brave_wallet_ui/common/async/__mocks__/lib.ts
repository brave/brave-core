import { mockBasicAttentionToken, mockEthToken } from '../../../stories/mock-data/mock-asset-options'
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

let mockBuyAssetList: BraveWallet.BlockchainToken[] = [
  mockEthToken,
  mockBasicAttentionToken
]

export const getBuyAssets = () => new Promise<BraveWallet.BlockchainToken[]>((resolve) => {
  resolve(mockBuyAssetList)
})

export const setMockedBuyAssets = (newList: BraveWallet.BlockchainToken[]) => {
  mockBuyAssetList = newList
}
