// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SimpleActionCreator } from 'redux-act'
import {
  AccountAssetOptionType,
  GetEthAddrReturnInfo,
  WalletAccountType,
  ER20TransferParams,
  SendTransactionParams,
  ERC721TransferFromParams
} from '../../constants/types'
import { isValidAddress } from '../../utils/address-utils'
import { getLocale } from '../../../common/locale'
import { toWeiHex } from '../../utils/format-balances'

export default function useSend (
  findENSAddress: (address: string) => Promise<GetEthAddrReturnInfo>,
  findUnstoppableDomainAddress: (address: string) => Promise<GetEthAddrReturnInfo>,
  selectedAccount: WalletAccountType,
  fromAsset: AccountAssetOptionType,
  sendERC20Transfer: SimpleActionCreator<ER20TransferParams>,
  sendTransaction: SimpleActionCreator<SendTransactionParams>,
  sendERC721TransferFrom: SimpleActionCreator<ERC721TransferFromParams>
) {
  const [toAddressOrUrl, setToAddressOrUrl] = React.useState('')
  const [toAddress, setToAddress] = React.useState('')
  const [addressError, setAddressError] = React.useState('')
  const [sendAmount, setSendAmount] = React.useState('')

  const supportedENSExtensions = ['.eth']
  const supportedUDExtensions = ['.crypto']

  const endsWithAny = (extensions: string[], url: string) => {
    return extensions.some(function (suffix) {
      return url.endsWith(suffix)
    })
  }

  const onSetToAddressOrUrl = (value: string) => {
    setToAddressOrUrl(value)
  }

  const onSetSendAmount = (value: string) => {
    setSendAmount(value)
  }

  const setNotRegisteredError = (url: string) => {
    setAddressError(getLocale('braveWalletNotDomain').replace('$1', url))
  }

  React.useEffect(() => {
    const valueToLowerCase = toAddressOrUrl.toLowerCase()

    // If value ends with a supported ENS extension, will call findENSAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedENSExtensions, valueToLowerCase)) {
      findENSAddress(toAddressOrUrl).then((value: GetEthAddrReturnInfo) => {
        if (value.success) {
          setAddressError('')
          setToAddress(value.address)
          return
        }
        setNotRegisteredError(valueToLowerCase)
      }).catch(e => console.log(e))
      return
    }

    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      findUnstoppableDomainAddress(toAddressOrUrl).then((value: GetEthAddrReturnInfo) => {
        if (value.success) {
          setAddressError('')
          setToAddress(value.address)
          return
        }
        setNotRegisteredError(valueToLowerCase)
      }).catch(e => console.log(e))
      return
    }

    // If value starts with 0x, will check if it's a valid address
    if (valueToLowerCase.startsWith('0x')) {
      setToAddress(toAddressOrUrl)
      isValidAddress(toAddressOrUrl, 20)
        ? setAddressError('')
        : setAddressError(getLocale('braveWalletNotValidEthAddress'))
      return
    }

    // Resets State
    if (toAddressOrUrl === '') {
      setAddressError('')
      setToAddress('')
      return
    }

    // Fallback error state
    setAddressError(getLocale('braveWalletNotValidAddress'))
  }, [toAddressOrUrl])

  const onSubmitSend = () => {
    fromAsset.asset.isErc20 && sendERC20Transfer({
      from: selectedAccount.address,
      to: toAddress,
      value: toWeiHex(sendAmount, fromAsset.asset.decimals),
      contractAddress: fromAsset.asset.contractAddress
    })

    fromAsset.asset.isErc721 && sendERC721TransferFrom({
      from: selectedAccount.address,
      to: toAddress,
      value: '',
      contractAddress: fromAsset.asset.contractAddress,
      tokenId: fromAsset.asset.tokenId ?? ''
    })

    !fromAsset.asset.isErc721 && !fromAsset.asset.isErc20 && sendTransaction({
      from: selectedAccount.address,
      to: toAddress,
      value: toWeiHex(sendAmount, fromAsset.asset.decimals)
    })

    setToAddressOrUrl('')
    setSendAmount('')
  }

  return {
    onSetSendAmount,
    onSetToAddressOrUrl,
    onSubmitSend,
    toAddressOrUrl,
    toAddress,
    sendAmount,
    addressError
  }
}
