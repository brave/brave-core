// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SimpleActionCreator } from 'redux-act'
import BigNumber from 'bignumber.js'

import {
  BraveWallet,
  GetEthAddrReturnInfo,
  WalletAccountType,
  ER20TransferParams,
  SendTransactionParams,
  ERC721TransferFromParams,
  GetChecksumEthAddressReturnInfo,
  AmountValidationErrorType
} from '../../constants/types'
import { isValidAddress } from '../../utils/address-utils'
import { getLocale } from '../../../common/locale'
import { toWeiHex } from '../../utils/format-balances'

export default function useSend (
  findENSAddress: (address: string) => Promise<GetEthAddrReturnInfo>,
  findUnstoppableDomainAddress: (address: string) => Promise<GetEthAddrReturnInfo>,
  getChecksumEthAddress: (address: string) => Promise<GetChecksumEthAddressReturnInfo>,
  sendAssetOptions: BraveWallet.BlockchainToken[],
  selectedAccount: WalletAccountType,
  sendERC20Transfer: SimpleActionCreator<ER20TransferParams>,
  sendTransaction: SimpleActionCreator<SendTransactionParams>,
  sendERC721TransferFrom: SimpleActionCreator<ERC721TransferFromParams>,
  fullTokenList: BraveWallet.BlockchainToken[]
) {
  const [selectedSendAsset, setSelectedSendAsset] = React.useState<BraveWallet.BlockchainToken>(sendAssetOptions[0])
  const [toAddressOrUrl, setToAddressOrUrl] = React.useState('')
  const [toAddress, setToAddress] = React.useState('')
  const [addressError, setAddressError] = React.useState('')
  const [addressWarning, setAddressWarning] = React.useState('')
  const [sendAmount, setSendAmount] = React.useState('')

  React.useEffect(() => {
    setSelectedSendAsset(sendAssetOptions[0])
  }, [sendAssetOptions])

  const onSelectSendAsset = (asset: BraveWallet.BlockchainToken) => {
    if (asset.isErc721) {
      setSendAmount('1')
    } else {
      setSendAmount('')
    }
    setSelectedSendAsset(asset)
  }

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

  const sendAmountValidationError: AmountValidationErrorType | undefined = React.useMemo(() => {
    if (!sendAmount) {
      return
    }

    return new BigNumber(sendAmount).times(10 ** selectedSendAsset.decimals).decimalPlaces() > 0
      ? 'fromAmountDecimalsOverflow'
      : undefined
  }, [sendAmount, selectedSendAsset])

  React.useEffect(() => {
    const valueToLowerCase = toAddressOrUrl.toLowerCase()

    // If value ends with a supported ENS extension, will call findENSAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedENSExtensions, valueToLowerCase)) {
      findENSAddress(toAddressOrUrl).then((value: GetEthAddrReturnInfo) => {
        if (value.error === BraveWallet.ProviderError.kSuccess) {
          setAddressError('')
          setAddressWarning('')
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
        if (value.error === BraveWallet.ProviderError.kSuccess) {
          setAddressError('')
          setAddressWarning('')
          setToAddress(value.address)
          return
        }
        setNotRegisteredError(valueToLowerCase)
      }).catch(e => console.log(e))
      return
    }

    // If value is the same as the selectedAccounts Wallet Address
    if (valueToLowerCase === selectedAccount?.address?.toLowerCase()) {
      setToAddress(toAddressOrUrl)
      setAddressWarning('')
      setAddressError(getLocale('braveWalletSameAddressError'))
      return
    }

    // If value is a Tokens Contract Address
    if (fullTokenList.some(token => token.contractAddress.toLowerCase() === valueToLowerCase)) {
      setToAddress(toAddressOrUrl)
      setAddressWarning('')
      setAddressError(getLocale('braveWalletContractAddressError'))
      return
    }

    // If value starts with 0x, will check if it's a valid address
    if (valueToLowerCase.startsWith('0x')) {
      setToAddress(toAddressOrUrl)
      if (!isValidAddress(toAddressOrUrl, 20)) {
        setAddressWarning('')
        setAddressError(getLocale('braveWalletNotValidEthAddress'))
        return
      }

      getChecksumEthAddress(toAddressOrUrl).then((value: GetChecksumEthAddressReturnInfo) => {
        const { checksumAddress } = value
        if (checksumAddress === toAddressOrUrl) {
          setAddressWarning('')
          setAddressError('')
          return
        }

        if ([toAddressOrUrl.toLowerCase(), toAddressOrUrl.toUpperCase()].includes(toAddressOrUrl)) {
          setAddressError('')
          setAddressWarning(getLocale('braveWalletAddressMissingChecksumInfoWarning'))
          return
        }

        setAddressWarning('')
        setAddressError(getLocale('braveWalletNotValidChecksumAddressError'))
      }).catch(e => console.log(e))

      return
    }

    // Resets State
    if (toAddressOrUrl === '' || selectedAccount.coin === BraveWallet.CoinType.FIL) {
      setAddressError('')
      setAddressWarning('')
      setToAddress('')
      return
    }
    // Fallback error state
    setAddressWarning('')
    setAddressError(getLocale('braveWalletNotValidAddress'))
  }, [toAddressOrUrl, selectedAccount])

  const onSubmitSend = () => {
    selectedSendAsset.isErc20 && sendERC20Transfer({
      from: selectedAccount.address,
      to: toAddress,
      value: toWeiHex(sendAmount, selectedSendAsset.decimals),
      contractAddress: selectedSendAsset.contractAddress
    })

    selectedSendAsset.isErc721 && sendERC721TransferFrom({
      from: selectedAccount.address,
      to: toAddress,
      value: '',
      contractAddress: selectedSendAsset.contractAddress,
      tokenId: selectedSendAsset.tokenId ?? ''
    })

    !selectedSendAsset.isErc721 && !selectedSendAsset.isErc20 && sendTransaction({
      from: selectedAccount.address,
      to: toAddress,
      value: toWeiHex(sendAmount, selectedSendAsset.decimals)
    })

    setToAddressOrUrl('')
    setSendAmount('')
  }

  return {
    onSetSendAmount,
    onSetToAddressOrUrl,
    onSubmitSend,
    onSelectSendAsset,
    toAddressOrUrl,
    toAddress,
    sendAmount,
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAmountValidationError
  }
}
