// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import {
  BraveWallet,
  GetEthAddrReturnInfo,
  GetUnstoppableDomainsWalletAddrReturnInfo,
  GetChecksumEthAddressReturnInfo,
  IsBase58EncodedSolanaPubkeyReturnInfo,
  AmountValidationErrorType,
  WalletState,
  SendFilTransactionParams,
  GetSolAddrReturnInfo,
  CoinTypesMap
} from '../../constants/types'
import { getLocale } from '../../../common/locale'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import { isValidAddress, isValidFilAddress } from '../../utils/address-utils'
import { endsWithAny } from '../../utils/string-utils'
import Amount from '../../utils/amount'

// hooks
import { useLib } from './useLib'
import { useAssets } from './assets'
import { useGetSelectedChainQuery } from '../slices/api.slice'

// constants
import {
  supportedENSExtensions,
  supportedSNSExtensions,
  supportedUDExtensions
} from '../constants/domain-extensions'

// ToDo: Remove isSendTab prop once we fully migrate to Send Tab
export default function useSend (isSendTab?: boolean) {
  // redux
  const dispatch = useDispatch()
  const {
    selectedAccount,
    fullTokenList,
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // custom hooks
  const {
    enableEnsOffchainLookup,
    findENSAddress,
    findSNSAddress,
    findUnstoppableDomainAddress,
    getChecksumEthAddress,
    isBase58EncodedSolanaPubkey
  } = useLib()
  const { sendAssetOptions } = useAssets()

  // State
  const [searchingForDomain, setSearchingForDomain] = React.useState<boolean>(false)
  const [showEnsOffchainWarning, setShowEnsOffchainWarning] = React.useState<boolean>(false)
  const [toAddressOrUrl, setToAddressOrUrl] = React.useState<string>('')
  const [toAddress, setToAddress] = React.useState<string>('')
  const [sendAmount, setSendAmount] = React.useState<string>('')
  const [addressError, setAddressError] = React.useState<string | undefined>(undefined)
  const [addressWarning, setAddressWarning] = React.useState<string | undefined>(undefined)
  const [selectedSendAsset, setSelectedSendAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(undefined)

  const selectSendAsset = (asset: BraveWallet.BlockchainToken | undefined) => {
    if (asset?.isErc721 || asset?.isNft) {
      setSendAmount('1')
    } else {
      setSendAmount('')
    }
    setToAddress('')
    setToAddressOrUrl('')
    setAddressError(undefined)
    setAddressWarning(undefined)
    setShowEnsOffchainWarning(false)
    setSearchingForDomain(false)
    setSelectedSendAsset(asset)
  }

  const setNotRegisteredError = React.useCallback(() => {
    setAddressError(getLocale('braveWalletNotDomain').replace('$1', CoinTypesMap[selectedNetwork?.coin ?? 0]))
  }, [selectedNetwork?.coin])

  const handleDomainLookupResponse = React.useCallback((addressOrUrl: string, error: BraveWallet.ProviderError, requireOffchainConsent: boolean) => {
    if (requireOffchainConsent) {
      setAddressError('')
      setAddressWarning('')
      setShowEnsOffchainWarning(true)
      setSearchingForDomain(false)
      return
    }
    if (addressOrUrl && error === BraveWallet.ProviderError.kSuccess) {
      setAddressError('')
      setAddressWarning('')
      setToAddress(addressOrUrl)
      // If found address is the same as the selectedAccounts Wallet Address
      if (addressOrUrl.toLowerCase() === selectedAccount?.address?.toLowerCase()) {
        setAddressError(getLocale('braveWalletSameAddressError'))
      }
      setSearchingForDomain(false)
      return
    }
    setShowEnsOffchainWarning(false)
    setNotRegisteredError()
    setSearchingForDomain(false)
  }, [selectedAccount?.address, setShowEnsOffchainWarning, setNotRegisteredError])

  const handleUDAddressLookUp = React.useCallback((addressOrUrl: string) => {
    setSearchingForDomain(true)
    setToAddress('')
    findUnstoppableDomainAddress(addressOrUrl, selectedSendAsset ?? null).then((value: GetUnstoppableDomainsWalletAddrReturnInfo) => {
      handleDomainLookupResponse(value.address, value.error, false)
    }).catch(e => console.log(e))
  }, [findUnstoppableDomainAddress, handleDomainLookupResponse, selectedSendAsset, selectedAccount?.coin])

  const processEthereumAddress = React.useCallback((addressOrUrl: string) => {
    const valueToLowerCase = addressOrUrl.toLowerCase()

    // If value ends with a supported ENS extension, will call findENSAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedENSExtensions, valueToLowerCase)) {
      setSearchingForDomain(true)
      setToAddress('')
      findENSAddress(addressOrUrl).then((value: GetEthAddrReturnInfo) => {
        handleDomainLookupResponse(value.address, value.error, value.requireOffchainConsent)
      }).catch(e => console.log(e))
      return
    }

    setShowEnsOffchainWarning(false)

    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      handleUDAddressLookUp(addressOrUrl)
      return
    }

    // If value is the same as the selectedAccounts Wallet Address
    if (valueToLowerCase === selectedAccount?.address?.toLowerCase()) {
      setToAddress(addressOrUrl)
      setAddressWarning('')
      setAddressError(getLocale('braveWalletSameAddressError'))
      return
    }

    // If value is a Tokens Contract Address
    if (fullTokenList.some(token => token.contractAddress.toLowerCase() === valueToLowerCase)) {
      setToAddress(addressOrUrl)
      setAddressWarning('')
      setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
      return
    }

    // If value starts with 0x, will check if it's a valid address
    if (valueToLowerCase.startsWith('0x')) {
      setToAddress(addressOrUrl)
      if (!isValidAddress(addressOrUrl, 20)) {
        setAddressWarning('')
        setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
        return
      }

      getChecksumEthAddress(addressOrUrl).then((value: GetChecksumEthAddressReturnInfo) => {
        const { checksumAddress } = value
        if (checksumAddress === addressOrUrl) {
          setAddressWarning('')
          setAddressError('')
          return
        }

        if ([addressOrUrl.toLowerCase(), addressOrUrl.toUpperCase()].includes(addressOrUrl)) {
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
      setShowEnsOffchainWarning(false)
      return
    }

    // Fallback error state
    setAddressWarning('')
    setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
  }, [selectedAccount?.address, handleUDAddressLookUp, handleDomainLookupResponse, setShowEnsOffchainWarning])

  const processFilecoinAddress = React.useCallback((addressOrUrl: string) => {
    const valueToLowerCase = addressOrUrl.toLowerCase()

    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      handleUDAddressLookUp(addressOrUrl)
      return
    }

    // If value is the same as the selectedAccounts Wallet Address
    if (valueToLowerCase === selectedAccount?.address.toLowerCase()) {
      setAddressWarning('')
      setAddressError(getLocale('braveWalletSameAddressError'))
      return
    }

    // Do nothing if value is an empty string
    if (addressOrUrl === '') {
      setAddressWarning('')
      setAddressError('')
      setToAddress('')
      return
    }

    setToAddress(valueToLowerCase)
    if (!isValidFilAddress(valueToLowerCase)) {
      setAddressWarning('')
      setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
      return
    }

    // Reset error and warning state back to normal
    setAddressWarning('')
    setAddressError('')
  }, [selectedAccount?.address, handleUDAddressLookUp])

  const processSolanaAddress = React.useCallback((addressOrUrl: string) => {
    const valueToLowerCase = addressOrUrl.toLowerCase()

    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      handleUDAddressLookUp(addressOrUrl)
      return
    }

    // If value ends with a supported SNS extension, will call findSNSAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedSNSExtensions, valueToLowerCase)) {
      setSearchingForDomain(true)
      setToAddress('')
      findSNSAddress(addressOrUrl).then((value: GetSolAddrReturnInfo) => {
        handleDomainLookupResponse(value.address, value.error, false)
      }).catch(e => console.log(e))
      return
    }

    setToAddress(addressOrUrl)

    // Do nothing if value is an empty string
    if (addressOrUrl === '') {
      setAddressWarning('')
      setAddressError('')
      return
    }

    // Check if value is the same as the sending address
    if (addressOrUrl.toLowerCase() === selectedAccount?.address?.toLowerCase()) {
      setAddressError(getLocale('braveWalletSameAddressError'))
      setAddressWarning('')
      return
    }

    // Check if value is a Tokens Contract Address
    if (fullTokenList.some(token => token.contractAddress.toLowerCase() === addressOrUrl.toLowerCase())) {
      setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
      setAddressWarning('')
      return
    }

    // Check if value is a Base58 Encoded Solana Pubkey
    isBase58EncodedSolanaPubkey(addressOrUrl).then((value: IsBase58EncodedSolanaPubkeyReturnInfo) => {
      const { result } = value

      // If result is false we show address error
      if (!result) {
        setAddressWarning('')
        setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
        return
      }
      setAddressWarning('')
      setAddressError('')
    }).catch(e => {
      console.log(e)
      // Reset state back to normal
      setAddressWarning('')
      setAddressError('')
    })
  }, [selectedAccount?.address, fullTokenList, handleUDAddressLookUp, handleDomainLookupResponse])

  const processAddressOrUrl = React.useCallback((addressOrUrl: string) => {
    if (selectedAccount?.coin === BraveWallet.CoinType.ETH) {
      processEthereumAddress(addressOrUrl)
    } else if (selectedAccount?.coin === BraveWallet.CoinType.FIL) {
      processFilecoinAddress(addressOrUrl)
    } else if (selectedAccount?.coin === BraveWallet.CoinType.SOL) {
      processSolanaAddress(addressOrUrl)
    }
  }, [
    selectedAccount?.coin,
    processEthereumAddress,
    processFilecoinAddress,
    processSolanaAddress
  ])

  const updateToAddressOrUrl = React.useCallback((addressOrUrl: string) => {
    setToAddressOrUrl(addressOrUrl)
    processAddressOrUrl(addressOrUrl)
  }, [processAddressOrUrl])

  const resetSendFields = React.useCallback((reselectSendAsset?: boolean) => {
    if (isSendTab) {
      selectSendAsset(undefined)
      setToAddressOrUrl('')
      setSendAmount('')
      return
    }
    if (reselectSendAsset) {
      selectSendAsset(sendAssetOptions[0])
    }
    setToAddressOrUrl('')
    setSendAmount('')
  }, [selectSendAsset, isSendTab, sendAssetOptions])

  const submitSend = React.useCallback(() => {
    if (!selectedSendAsset) {
      console.log('Failed to submit Send transaction: no send asset selected')
      return
    }

    if (!selectedAccount) {
      console.log('Failed to submit Send transaction: no account selected')
      return
    }

    selectedSendAsset.isErc20 && dispatch(WalletActions.sendERC20Transfer({
      from: selectedAccount.address,
      to: toAddress,
      value: new Amount(sendAmount)
        .multiplyByDecimals(selectedSendAsset.decimals) // ETH → Wei conversion
        .toHex(),
      contractAddress: selectedSendAsset.contractAddress,
      coin: selectedAccount.coin
    }))

    selectedSendAsset.isErc721 && dispatch(WalletActions.sendERC721TransferFrom({
      from: selectedAccount.address,
      to: toAddress,
      value: '',
      contractAddress: selectedSendAsset.contractAddress,
      tokenId: selectedSendAsset.tokenId ?? '',
      coin: selectedAccount.coin
    }))

    if (
      selectedAccount.coin === BraveWallet.CoinType.SOL &&
      selectedSendAsset.contractAddress !== '' &&
      !selectedSendAsset.isErc20 && !selectedSendAsset.isErc721
    ) {
      dispatch(WalletActions.sendSPLTransfer({
        from: selectedAccount.address,
        to: toAddress,
        value: !selectedSendAsset.isNft ? new Amount(sendAmount)
          .multiplyByDecimals(selectedSendAsset.decimals)
          .toHex() : new Amount(sendAmount).toHex(),
        coin: selectedAccount.coin,
        splTokenMintAddress: selectedSendAsset.contractAddress
      }))
      resetSendFields(true)
      return
    }

    if (selectedAccount.coin === BraveWallet.CoinType.FIL) {
      dispatch(WalletActions.sendTransaction({
        from: selectedAccount.address,
        to: toAddress,
        value: new Amount(sendAmount)
          .multiplyByDecimals(selectedSendAsset.decimals).toNumber().toString(),
        coin: selectedAccount.coin
      } as SendFilTransactionParams))
      resetSendFields()
      return
    }

    if (selectedSendAsset.isErc721 || selectedSendAsset.isErc20) {
      if (isSendTab) {
        resetSendFields()
      }
      return
    }

    dispatch(WalletActions.sendTransaction({
      from: selectedAccount.address,
      to: toAddress,
      coin: selectedAccount.coin,
      value: selectedAccount.coin === BraveWallet.CoinType.FIL
        ? new Amount(sendAmount).multiplyByDecimals(selectedSendAsset.decimals).toString()
        : new Amount(sendAmount).multiplyByDecimals(selectedSendAsset.decimals).toHex()
    }))

    resetSendFields()
  }, [
    selectedSendAsset,
    selectedAccount,
    sendAmount,
    toAddress,
    resetSendFields,
    isSendTab
  ])

  // memos
  const sendAmountValidationError: AmountValidationErrorType | undefined = React.useMemo(() => {
    if (!sendAmount || !selectedSendAsset) {
      return
    }

    const amountBN = new Amount(sendAmount)
      .multiplyByDecimals(selectedSendAsset.decimals) // ETH → Wei conversion
      .value // extract BigNumber object wrapped by Amount

    const amountDP = amountBN && amountBN.decimalPlaces()
    return amountDP && amountDP > 0
      ? 'fromAmountDecimalsOverflow'
      : undefined
  }, [sendAmount, selectedSendAsset])

  // effects
  React.useEffect(() => {
    if (isSendTab) {
      return
    }
    // We also check that coinType matches here because localhost
    // networks share the same chainId
    if (
      selectedSendAsset?.chainId === selectedNetwork?.chainId &&
      selectedSendAsset?.coin === selectedNetwork?.coin
    ) {
      return
    }
    selectSendAsset(sendAssetOptions[0])
  }, [sendAssetOptions, selectedSendAsset, selectedNetwork, isSendTab])

  return {
    setSendAmount,
    updateToAddressOrUrl,
    submitSend,
    selectSendAsset,
    toAddressOrUrl,
    toAddress,
    sendAmount,
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAmountValidationError,
    showEnsOffchainWarning,
    setShowEnsOffchainWarning,
    enableEnsOffchainLookup,
    searchingForDomain,
    processAddressOrUrl
  }
}
