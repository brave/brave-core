// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert_ts.js';
import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query'

// Types
import {
  BraveWallet,
  GetEthAddrReturnInfo,
  GetUnstoppableDomainsWalletAddrReturnInfo,
  IsBase58EncodedSolanaPubkeyReturnInfo,
  AmountValidationErrorType,
  GetSolAddrReturnInfo,
  CoinTypesMap,
  BaseTransactionParams
} from '../../constants/types'

// Utils
import { getLocale } from '../../../common/locale'
import { isValidAddress, isValidFilAddress } from '../../utils/address-utils'
import { endsWithAny } from '../../utils/string-utils'
import Amount from '../../utils/amount'
import { WalletSelectors } from '../selectors'

// hooks
import { useLib } from './useLib'
import { useAssets } from './assets'
import { useGetFVMAddressQuery, useGetSelectedChainQuery, walletApi } from '../slices/api.slice'
import { useUnsafeWalletSelector } from './use-safe-selector'
import { useSelectedAccountQuery } from '../slices/api.slice.extra'

// constants
import {
  supportedENSExtensions,
  supportedSNSExtensions,
  supportedUDExtensions
} from '../constants/domain-extensions'
import { getChecksumEthAddress } from '../async/lib'

export function useSend () {
  // redux
  const dispatch = useDispatch()
  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()

  // custom hooks
  const {
    enableEnsOffchainLookup,
    findENSAddress,
    findSNSAddress,
    findUnstoppableDomainAddress,
    isBase58EncodedSolanaPubkey
  } = useLib()
  const sendAssetOptions = useAssets()

  // State
  const [searchingForDomain, setSearchingForDomain] = React.useState<boolean>(false)
  const [showEnsOffchainWarning, setShowEnsOffchainWarning] = React.useState<boolean>(false)
  const [toAddressOrUrl, setToAddressOrUrl] = React.useState<string>('')
  const [toAddress, setToAddress] = React.useState<string>('')
  const [sendAmount, setSendAmount] = React.useState<string>('')
  const [addressError, setAddressError] = React.useState<string | undefined>(undefined)
  const [addressWarning, setAddressWarning] = React.useState<string | undefined>(undefined)
  const [selectedSendAsset, setSelectedSendAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(undefined)
  const [showFilecoinFEVMWarning, setShowFilecoinFEVMWarning] = React.useState<boolean>(false)
  const { data: fevmTranslatedAddresses } = useGetFVMAddressQuery(
    selectedSendAsset?.coin === BraveWallet.CoinType.FIL
      ? {
          coin: selectedSendAsset?.coin,
          addresses: [toAddressOrUrl],
          isMainNet: selectedSendAsset?.chainId === BraveWallet.FILECOIN_MAINNET
        }
      : skipToken
  )

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
    setShowFilecoinFEVMWarning(false)
  }

  const validateETHAddress = React.useCallback(async (address: string) => {
    if (!isValidAddress(address, 20)) {
      setAddressWarning('')
      setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
      return false
    }

    const { checksumAddress } = (await getChecksumEthAddress(address))
      if (checksumAddress === address) {
        setAddressWarning('')
        setAddressError('')
        return true
      }

      if ([address.toLowerCase(), address.toUpperCase()].includes(address)) {
        setAddressError('')
        setAddressWarning(getLocale('braveWalletAddressMissingChecksumInfoWarning'))
        return false
      }
      setAddressWarning('')
      setAddressError(getLocale('braveWalletNotValidChecksumAddressError'))
      return false
  }, [setAddressWarning, setAddressError])

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
  }, [findUnstoppableDomainAddress, handleDomainLookupResponse, selectedSendAsset, selectedAccount])

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

    if (selectedAccount && selectedSendAsset &&
      selectedAccount.accountId.coin === BraveWallet.CoinType.ETH &&
      (selectedSendAsset.chainId === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID ||
        selectedSendAsset.chainId === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID) &&
      isValidFilAddress(addressOrUrl)) {
        setToAddress(addressOrUrl);
        setAddressWarning('')
        setAddressError('')
        return;
    }

    // If value starts with 0x, will check if it's a valid address
    if (valueToLowerCase.startsWith('0x')) {
      setToAddress(addressOrUrl)
      validateETHAddress(addressOrUrl)
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
  }, [selectedAccount,
      selectedSendAsset,
      handleUDAddressLookUp,
      handleDomainLookupResponse,
      setShowEnsOffchainWarning])

  const processFilecoinAddress = React.useCallback(async (addressOrUrl: string) => {
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

    // If value starts with 0x, will check if it's a valid address
    if (valueToLowerCase.startsWith('0x')) {
      setToAddress(addressOrUrl)
      const v = (await validateETHAddress(addressOrUrl))
      setShowFilecoinFEVMWarning(v)
      return
    } else {
      setToAddress(valueToLowerCase)
      if (!isValidFilAddress(valueToLowerCase)) {
        setAddressWarning('')
        setAddressError(getLocale('braveWalletInvalidRecipientAddress'))
        return
      }
    }
    // Reset error and warning state back to normal
    setAddressWarning('')
    setAddressError('')
  }, [selectedAccount?.address, fevmTranslatedAddresses, validateETHAddress, handleUDAddressLookUp])

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

  const processBitcoinAddress = React.useCallback((addressOrUrl: string) => {
    // TODO(apaymyshev): support btc address aliases(UD, SNS)

    setToAddress(addressOrUrl)

    // Do nothing if value is an empty string
    if (addressOrUrl === '') {
      setAddressWarning('')
      setAddressError('')
      // eslint-disable-next-line no-useless-return
      return
    }

    // Check if value is the same as the sending address
    // TODO(apaymyshev): should prohibit self transfers?

    // TODO(apaymyshev): validate address format.
  }, [])

  const processAddressOrUrl = React.useCallback((addressOrUrl: string) => {
    if (!selectedAccount) {
      return
    }

    setShowFilecoinFEVMWarning(false)

    if (selectedAccount.accountId.coin === BraveWallet.CoinType.ETH) {
      processEthereumAddress(addressOrUrl)
    } else if (selectedAccount.accountId.coin === BraveWallet.CoinType.FIL) {
      processFilecoinAddress(addressOrUrl)
    } else if (selectedAccount.accountId.coin === BraveWallet.CoinType.SOL) {
      processSolanaAddress(addressOrUrl)
    } else if (selectedAccount.accountId.coin === BraveWallet.CoinType.BTC) {
      processBitcoinAddress(addressOrUrl)
    } else {
      assertNotReached(`Unknown coin ${selectedAccount.accountId.coin}`)
    }
  }, [
    selectedAccount,
    processEthereumAddress,
    processFilecoinAddress,
    processSolanaAddress,
    processBitcoinAddress
  ])

  const updateToAddressOrUrl = React.useCallback((addressOrUrl: string) => {
    setToAddressOrUrl(addressOrUrl)
    processAddressOrUrl(addressOrUrl)
  }, [processAddressOrUrl])

  const resetSendFields = React.useCallback(() => {
    selectSendAsset(undefined)
    setToAddressOrUrl('')
    setSendAmount('')
    setShowFilecoinFEVMWarning(false)
  }, [selectSendAsset])

  const submitSend = React.useCallback(() => {
    if (!selectedSendAsset) {
      console.log('Failed to submit Send transaction: no send asset selected')
      return
    }

    if (!selectedAccount) {
      console.log('Failed to submit Send transaction: no account selected')
      return
    }

    if (!selectedNetwork) {
      console.log('Failed to submit Send transaction: no network selected')
      return
    }

    const fromAccount: BaseTransactionParams['fromAccount'] = {
      accountId: selectedAccount.accountId,
      address: selectedAccount.address,
      hardware: selectedAccount.hardware,
    }

    selectedSendAsset.isErc20 &&
      dispatch(
        walletApi.endpoints.sendERC20Transfer.initiate({
          network: selectedNetwork,
          fromAccount,
          to: toAddress,
          value: new Amount(sendAmount)
            // ETH → Wei conversion
            .multiplyByDecimals(selectedSendAsset.decimals)
            .toHex(),
          contractAddress: selectedSendAsset.contractAddress,
        })
      )

    selectedSendAsset.isErc721 &&
      dispatch(
        walletApi.endpoints.sendERC721TransferFrom.initiate({
          network: selectedNetwork,
          fromAccount,
          to: toAddress,
          value: '',
          contractAddress: selectedSendAsset.contractAddress,
          tokenId: selectedSendAsset.tokenId ?? '',
        })
      )

    if (
      selectedAccount.accountId.coin === BraveWallet.CoinType.SOL &&
      selectedSendAsset.contractAddress !== '' &&
      !selectedSendAsset.isErc20 &&
      !selectedSendAsset.isErc721
    ) {
      dispatch(
        walletApi.endpoints.sendSPLTransfer.initiate({
          network: selectedNetwork,
          fromAccount,
          to: toAddress,
          value: !selectedSendAsset.isNft
            ? new Amount(sendAmount)
                .multiplyByDecimals(selectedSendAsset.decimals)
                .toHex()
            : new Amount(sendAmount).toHex(),
          splTokenMintAddress: selectedSendAsset.contractAddress
        })
      )
      resetSendFields()
      return
    }

    if (selectedAccount.accountId.coin === BraveWallet.CoinType.FIL) {
      dispatch(
        walletApi.endpoints.sendTransaction.initiate({
          network: selectedNetwork,
          fromAccount,
          to: toAddress,
          value: new Amount(sendAmount)
            .multiplyByDecimals(selectedSendAsset.decimals)
            .toNumber()
            .toString(),
        })
      )
      resetSendFields()
      return
    }

    if (selectedSendAsset.isErc721 || selectedSendAsset.isErc20) {
      resetSendFields()
      return
    }

    if (selectedAccount.accountId.coin === BraveWallet.CoinType.ETH &&
      (selectedSendAsset.chainId === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID ||
        selectedSendAsset.chainId === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID) &&
      isValidFilAddress(toAddress)) {

        dispatch(
          walletApi.endpoints.sendETHFilForwarderTransfer.initiate({
            network: selectedNetwork,
            fromAccount,
            to: toAddress,
            value: new Amount(sendAmount)
              // ETH → Wei conversion
              .multiplyByDecimals(selectedSendAsset.decimals)
              .toHex(),
            contractAddress: "0x2b3ef6906429b580b7b2080de5ca893bc282c225"
          })
        )
      resetSendFields()
      return
    }

    dispatch(
      walletApi.endpoints.sendTransaction.initiate({
        network: selectedNetwork,
        fromAccount,
        to: toAddress,
        value:
          selectedAccount.accountId.coin === BraveWallet.CoinType.FIL
            ? new Amount(sendAmount)
                .multiplyByDecimals(selectedSendAsset.decimals)
                .toString()
            : new Amount(sendAmount)
                .multiplyByDecimals(selectedSendAsset.decimals)
                .toHex()
      })
    )

    resetSendFields()
  }, [
    selectedSendAsset,
    selectedAccount,
    selectedNetwork,
    sendAmount,
    toAddress,
    resetSendFields
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
    showFilecoinFEVMWarning,
    fevmTranslatedAddresses,
    showEnsOffchainWarning,
    setShowEnsOffchainWarning,
    enableEnsOffchainLookup,
    searchingForDomain,
    processAddressOrUrl,
    sendAssetOptions
  }
}
export default useSend
