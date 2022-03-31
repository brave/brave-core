// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SimpleActionCreator } from 'redux-act'

import {
  BraveWallet,
  GetEthAddrReturnInfo,
  WalletAccountType,
  ER20TransferParams,
  SendEthTransactionParams,
  SendFilTransactionParams,
  ERC721TransferFromParams,
  GetChecksumEthAddressReturnInfo,
  AmountValidationErrorType
} from '../../constants/types'
import { getLocale } from '../../../common/locale'

// Utils
import { isValidAddress, isValidFilAddress } from '../../utils/address-utils'
import Amount from '../../utils/amount'

export default function useSend (
  findENSAddress: (address: string) => Promise<GetEthAddrReturnInfo>,
  findUnstoppableDomainAddress: (address: string) => Promise<GetEthAddrReturnInfo>,
  getChecksumEthAddress: (address: string) => Promise<GetChecksumEthAddressReturnInfo>,
  sendAssetOptions: BraveWallet.BlockchainToken[],
  selectedAccount: WalletAccountType,
  sendERC20Transfer: SimpleActionCreator<ER20TransferParams>,
  sendTransaction: SimpleActionCreator<SendEthTransactionParams | SendFilTransactionParams>,
  sendERC721TransferFrom: SimpleActionCreator<ERC721TransferFromParams>,
  fullTokenList: BraveWallet.BlockchainToken[]
) {
  // selectedSendAsset can be undefined if sendAssetOptions is an empty array.
  const [selectedSendAsset, setSelectedSendAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(sendAssetOptions[0])
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
    if (!sendAmount || !selectedSendAsset) {
      return
    }

    const amountBN = new Amount(sendAmount)
      .multiplyByDecimals(selectedSendAsset.decimals) // ETH → Wei conversion
      .value // extract BigNumber object wrapped by Amount
    return amountBN && amountBN.decimalPlaces() > 0
      ? 'fromAmountDecimalsOverflow'
      : undefined
  }, [sendAmount, selectedSendAsset])

  const processEthereumAddress = React.useCallback((toAddressOrUrl: string) => {
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
        setAddressError(getLocale('braveWalletNotValidAddress'))
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
    if (valueToLowerCase === '') {
      setAddressError('')
      setAddressWarning('')
      setToAddress('')
      return
    }

    // Fallback error state
    setAddressWarning('')
    setAddressError(getLocale('braveWalletNotValidAddress'))
  }, [selectedAccount])

  const processFilecoinAddress = React.useCallback((toAddressOrUrl: string) => {
    const valueToLowerCase = toAddressOrUrl.toLowerCase()
    setToAddress(valueToLowerCase)
    if (!isValidFilAddress(valueToLowerCase)) {
      setAddressWarning('')
      setAddressError(getLocale('braveWalletNotValidFilAddress'))
      return
    }
    setAddressWarning('')
    setAddressError('')
  }, [])

  React.useEffect(() => {
    if (selectedAccount?.coin === BraveWallet.CoinType.ETH) {
      processEthereumAddress(toAddressOrUrl)
    } else if (selectedAccount?.coin === BraveWallet.CoinType.FIL) {
      processFilecoinAddress(toAddressOrUrl)
    }
  }, [toAddressOrUrl, selectedAccount])

  const onSubmitSend = () => {
    if (!selectedSendAsset) {
      console.log('Failed to submit Send transaction: no send asset selected')
      return
    }

    selectedSendAsset.isErc20 && sendERC20Transfer({
      from: selectedAccount.address,
      to: toAddress,
      value: new Amount(sendAmount)
        .multiplyByDecimals(selectedSendAsset.decimals) // ETH → Wei conversion
        .toHex(),
      contractAddress: selectedSendAsset.contractAddress,
      coin: selectedAccount.coin
    })

    selectedSendAsset.isErc721 && sendERC721TransferFrom({
      from: selectedAccount.address,
      to: toAddress,
      value: '',
      contractAddress: selectedSendAsset.contractAddress,
      tokenId: selectedSendAsset.tokenId ?? '',
      coin: selectedAccount.coin
    })

    if (selectedSendAsset.isErc721 || selectedSendAsset.isErc20) { return }
    if (selectedAccount.coin === BraveWallet.CoinType.ETH) {
      sendTransaction({
        from: selectedAccount.address,
        to: toAddress,
        value: new Amount(sendAmount)
          .multiplyByDecimals(selectedSendAsset.decimals)
          .toHex(),
        coin: selectedAccount.coin
      } as SendEthTransactionParams)
    } else if (selectedAccount.coin === BraveWallet.CoinType.FIL) {
      sendTransaction({
        from: selectedAccount.address,
        to: toAddress,
        value: new Amount(sendAmount)
          .multiplyByDecimals(selectedSendAsset.decimals).toString(),
        coin: selectedAccount.coin
      } as SendFilTransactionParams)
    }

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
