import { mockBasicAttentionToken, mockEthToken } from '../../../stories/mock-data/mock-asset-options'
import { BraveWallet, GetChecksumEthAddressReturnInfo, GetEthAddrReturnInfo } from '../../../constants/types'

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

const mockENSValues = [
  {
    address: 'mockAddress2',
    name: 'brave.eth'
  },
  {
    address: 'mockAddress3',
    name: 'bravey.eth'
  }
]

const mockUDValues = [
  {
    address: 'mockAddress2',
    name: 'brave.crypto'
  },
  {
    address: 'mockAddress3',
    name: 'bravey.crypto'
  }
]

export const getBuyAssets = () => new Promise<BraveWallet.BlockchainToken[]>((resolve) => {
  resolve(mockBuyAssetList)
})

export const setMockedBuyAssets = (newList: BraveWallet.BlockchainToken[]) => {
  mockBuyAssetList = newList
}

export const findENSAddress = async (address: string) => {
  const foundAddress = mockENSValues.find((value) => value.name === address)
  if (foundAddress) {
    return { address: foundAddress.address, error: 0, errorMessage: '' } as GetEthAddrReturnInfo
  }
  return { address: '', error: 1, errorMessage: '' } as GetEthAddrReturnInfo
}

export const findUnstoppableDomainAddress = async (address: string) => {
  const foundAddress = mockUDValues.find((value) => value.name === address)
  if (foundAddress) {
    return { address: foundAddress.address, error: 0, errorMessage: '' } as GetEthAddrReturnInfo
  }
  return { address: '', error: 1, errorMessage: '' } as GetEthAddrReturnInfo
}

export const getChecksumEthAddress = async () => {
  return {} as GetChecksumEthAddressReturnInfo
}
