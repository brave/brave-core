// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  BraveWallet,
  GetBlockchainTokenInfoReturnInfo
} from '../../constants/types'

export default function useTokenInfo (
  getBlockchainTokenInfo: (address: string) => Promise<GetBlockchainTokenInfoReturnInfo>,
  visibleTokens: BraveWallet.BlockchainToken[],
  fullTokenList: BraveWallet.BlockchainToken[],
  selectedNetwork?: BraveWallet.NetworkInfo
) {
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')
  const [foundTokenInfoByContractAddress, setFoundTokenInfoByContractAddress] = React.useState<BraveWallet.BlockchainToken | undefined>()

  // Instead of having this be a useCallback hook here we are using useEffect to
  // handle the asynchronous getBlockchainTokenInfo fallback method.
  // That away each component that uses this hook will not have to handle
  // this async call individually.
  React.useEffect(() => {
    const contractAddress = tokenContractAddress.toLowerCase()
    if (contractAddress === '') {
      setFoundTokenInfoByContractAddress(undefined)
      return
    }

    const checkVisibleList = visibleTokens.find((token) => token.contractAddress.toLowerCase() === contractAddress)
    const checkedLists = checkVisibleList ?? (fullTokenList.find((token) => token.contractAddress.toLowerCase() === contractAddress))

    if (checkedLists) {
      setFoundTokenInfoByContractAddress(checkedLists)
      return
    }

    if (!checkedLists && selectedNetwork?.chainId === BraveWallet.MAINNET_CHAIN_ID) {
      getBlockchainTokenInfo(contractAddress).then((value: GetBlockchainTokenInfoReturnInfo) => {
        if (value.token) {
          setFoundTokenInfoByContractAddress(value.token)
          return
        }
        setFoundTokenInfoByContractAddress(undefined)
      }).catch(e => console.log(e))
    }
    setFoundTokenInfoByContractAddress(undefined)
  }, [tokenContractAddress, visibleTokens, fullTokenList, selectedNetwork?.chainId])
  return {
    onFindTokenInfoByContractAddress: setTokenContractAddress,
    foundTokenInfoByContractAddress
  }
}
