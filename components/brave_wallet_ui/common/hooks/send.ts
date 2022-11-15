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
  GetSolAddrReturnInfo
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
import { PendingCryptoSendState, SendCryptoActions } from '../reducers/send_crypto_reducer'

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
    selectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)
  const {
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAmount,
    toAddress,
    toAddressOrUrl,
    showEnsOffchainLookupOptions,
    ensOffchainLookupOptions
  } = useSelector((state: { sendCrypto: PendingCryptoSendState }) => state.sendCrypto)

  // custom hooks
  const {
    findENSAddress,
    findSNSAddress,
    findUnstoppableDomainAddress,
    getChecksumEthAddress,
    isBase58EncodedSolanaPubkey
  } = useLib()
  const { sendAssetOptions } = useAssets()

  // methods
  const setToAddress = (payload?: string | undefined) => {
    dispatch(SendCryptoActions.setToAddress(payload))
  }
  const setShowEnsOffchainLookupOptions = (payload: boolean) => {
    dispatch(SendCryptoActions.setShowEnsOffchainLookupOptions(payload))
  }
  const setEnsOffchainLookupOptions = (payload?: BraveWallet.EnsOffchainLookupOptions | undefined) => {
    dispatch(SendCryptoActions.setEnsOffchainLookupOptions(payload))
  }
  const setAddressWarning = (payload?: string | undefined) => {
    dispatch(SendCryptoActions.setAddressWarning(payload))
  }
  const setAddressError = (payload?: string | undefined) => {
    dispatch(SendCryptoActions.setAddressError(payload))
  }
  const setSendAmount = (payload?: string | undefined) => {
    dispatch(SendCryptoActions.setSendAmount(payload))
  }
  const setToAddressOrUrl = (payload?: string | undefined) => {
    dispatch(SendCryptoActions.setToAddressOrUrl(payload))
  }

  const selectSendAsset = (asset: BraveWallet.BlockchainToken | undefined) => {
    if (asset?.isErc721 || asset?.isNft) {
      setSendAmount('1')
    } else {
      setSendAmount('')
    }
    dispatch(SendCryptoActions.selectSendAsset(asset))
  }

  const setNotRegisteredError = (url: string) => {
    setAddressError(getLocale('braveWalletNotDomain').replace('$1', url))
  }

  const processEthereumAddress = React.useCallback((toAddressOrUrl: string) => {
    const valueToLowerCase = toAddressOrUrl.toLowerCase()
    // If value ends with a supported ENS extension, will call findENSAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedENSExtensions, valueToLowerCase)) {
      findENSAddress(toAddressOrUrl, ensOffchainLookupOptions).then((value: GetEthAddrReturnInfo) => {
        if (value.error === BraveWallet.ProviderError.kSuccess) {
          setAddressError('')
          setAddressWarning('')
          setToAddress(value.address)
          setShowEnsOffchainLookupOptions(value.requireOffchainConsent)
          return
        }
        setShowEnsOffchainLookupOptions(false)
        setNotRegisteredError(valueToLowerCase)
      }).catch(e => console.log(e))
      return
    }
    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      findUnstoppableDomainAddress(toAddressOrUrl, selectedSendAsset ?? null).then((value: GetUnstoppableDomainsWalletAddrReturnInfo) => {
        if (value.address && value.error === BraveWallet.ProviderError.kSuccess) {
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
  }, [selectedAccount?.address, selectedSendAsset, ensOffchainLookupOptions])

  const processFilecoinAddress = React.useCallback((toAddressOrUrl: string) => {
    const valueToLowerCase = toAddressOrUrl.toLowerCase()

    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      findUnstoppableDomainAddress(toAddressOrUrl, selectedSendAsset ?? null).then((value: GetUnstoppableDomainsWalletAddrReturnInfo) => {
        if (value.address && value.error === BraveWallet.ProviderError.kSuccess) {
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
    if (valueToLowerCase === selectedAccount?.address.toLowerCase()) {
      setAddressWarning('')
      setAddressError(getLocale('braveWalletSameAddressError'))
      return
    }

    // Do nothing if value is an empty string
    if (toAddressOrUrl === '') {
      setAddressWarning('')
      setAddressError('')
      setToAddress('')
      return
    }

    setToAddress(valueToLowerCase)
    if (!isValidFilAddress(valueToLowerCase)) {
      setAddressWarning('')
      setAddressError(getLocale('braveWalletNotValidFilAddress'))
      return
    }

    // Reset error and warning state back to normal
    setAddressWarning('')
    setAddressError('')
  }, [selectedSendAsset, selectedAccount?.address])

  const processSolanaAddress = React.useCallback((toAddressOrUrl: string) => {
    const valueToLowerCase = toAddressOrUrl.toLowerCase()

    // If value ends with a supported UD extension, will call findUnstoppableDomainAddress.
    // If success true, will set toAddress else will return error message.
    if (endsWithAny(supportedUDExtensions, valueToLowerCase)) {
      findUnstoppableDomainAddress(toAddressOrUrl, selectedSendAsset ?? null).then((value: GetUnstoppableDomainsWalletAddrReturnInfo) => {
        if (value.address && value.error === BraveWallet.ProviderError.kSuccess) {
          setAddressError('')
          setAddressWarning('')
          setToAddress(value.address)
          return
        }
        setNotRegisteredError(valueToLowerCase)
      }).catch(e => console.log(e))
      return
    }

    if (endsWithAny(supportedSNSExtensions, valueToLowerCase)) {
      findSNSAddress(toAddressOrUrl).then((value: GetSolAddrReturnInfo) => {
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

    setToAddress(toAddressOrUrl)

    // Do nothing if value is an empty string
    if (toAddressOrUrl === '') {
      setAddressWarning('')
      setAddressError('')
      return
    }

    // Check if value is the same as the sending address
    if (toAddressOrUrl.toLowerCase() === selectedAccount?.address?.toLowerCase()) {
      setAddressError(getLocale('braveWalletSameAddressError'))
      setAddressWarning('')
      return
    }

    // Check if value is a Tokens Contract Address
    if (fullTokenList.some(token => token.contractAddress.toLowerCase() === toAddressOrUrl.toLowerCase())) {
      setAddressError(getLocale('braveWalletContractAddressError'))
      setAddressWarning('')
      return
    }

    // Check if value is a Base58 Encoded Solana Pubkey
    isBase58EncodedSolanaPubkey(toAddressOrUrl).then((value: IsBase58EncodedSolanaPubkeyReturnInfo) => {
      const { result } = value

      // If result is false we show address error
      if (!result) {
        setAddressWarning('')
        setAddressError(getLocale('braveWalletNotValidSolAddress'))
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
  }, [selectedAccount?.address, selectedSendAsset, fullTokenList])

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

    return amountBN && amountBN.decimalPlaces() > 0
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

  React.useEffect(() => {
    if (selectedAccount?.coin === BraveWallet.CoinType.ETH) {
      processEthereumAddress(toAddressOrUrl)
    } else if (selectedAccount?.coin === BraveWallet.CoinType.FIL) {
      processFilecoinAddress(toAddressOrUrl)
    } else if (selectedAccount?.coin === BraveWallet.CoinType.SOL) {
      processSolanaAddress(toAddressOrUrl)
    }
  }, [
    toAddressOrUrl,
    selectedAccount?.coin,
    ensOffchainLookupOptions,
    processEthereumAddress,
    processFilecoinAddress,
    processSolanaAddress
  ])

  return {
    setSendAmount,
    setToAddressOrUrl,
    submitSend,
    selectSendAsset,
    toAddressOrUrl,
    toAddress,
    sendAmount,
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAmountValidationError,
    showEnsOffchainLookupOptions,
    ensOffchainLookupOptions,
    setEnsOffchainLookupOptions
  }
}
