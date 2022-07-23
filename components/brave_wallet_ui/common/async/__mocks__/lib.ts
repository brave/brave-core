// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { mockBasicAttentionToken, mockEthToken } from '../../../stories/mock-data/mock-asset-options'
import { BraveWallet, GetChecksumEthAddressReturnInfo, GetEthAddrReturnInfo } from '../../../constants/types'
import { HardwareWalletConnectOpts } from '../../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'

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

export const isStrongPassword = (value: string) => {
  return (
    (value.length > 7) && // is at least 7 characters
    /[-’/`~!#*$@_%+=.,^&(){}[\]|;:”<>?\\]/g.test(value) && // contains a special character
    value.toLowerCase() !== value && // contains an uppercase character
    /\d/.test(value) // contains a number
  )
}

export const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Promise<BraveWallet.HardwareWalletAccount[]> => {
  const makeDerivationPath = (index: number): string => `m/44'/60'/${index}'/0/0`

  return new Promise((resolve) => {
    resolve(Array.from({ length: opts.stopIndex - opts.startIndex }, (_, i) => ({
      address: '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2',
      derivationPath: makeDerivationPath(i + opts.startIndex),
      name: 'Ledger 1',
      hardwareVendor: 'Ledger',
      deviceId: 'device1',
      coin: BraveWallet.CoinType.ETH,
      network: undefined
    })))
  })
}

export const getBalance = (): Promise<string> => {
  return new Promise(async (resolve) => {
    resolve('0')
  })
}
