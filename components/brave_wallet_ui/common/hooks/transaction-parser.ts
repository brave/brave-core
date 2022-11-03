/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as Solana from '@solana/web3.js'
import { useSelector } from 'react-redux'

// Constants
import {
  BraveWallet,
  SolFeeEstimates,
  TimeDelta,
  WalletAccountType,
  WalletState
} from '../../constants/types'
import {
  MAX_UINT256,
  NATIVE_ASSET_CONTRACT_ADDRESS_0X
} from '../constants/magics'
import { SwapExchangeProxy } from '../constants/registry'

// Utils
import { getLocale } from '../../../common/locale'
import Amount from '../../utils/amount'
import { getTypedSolanaTxInstructions, TypedSolanaInstructionWithParams } from '../../utils/solana-instruction-utils'
import {
  getTransactionInteractionAddress,
  getTransactionNonce,
  getTransactionToAddress,
  isFilecoinTransaction,
  isSolanaDappTransaction,
  isSolanaTransaction
} from '../../utils/tx-utils'
import { getBalance } from '../../utils/balance-utils'
import { getAddressLabel } from '../../utils/account-utils'
import { toProperCase } from '../../utils/string-utils'
import { makeNetworkAsset } from '../../options/asset-options'

// Hooks
import usePricing from './pricing'

type SolanaParamsWithLamports = Solana.CreateAccountParams | Solana.CreateAccountWithSeedParams | Solana.TransferParams | Solana.TransferWithSeedParams | Solana.WithdrawNonceParams

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
  missingGasLimitError?: string
  gasPremium?: string
  gasFeeCap?: string
}

export interface ParsedTransaction extends ParsedTransactionFees {
  // Common fields
  hash: string
  nonce: string
  createdTime: TimeDelta
  status: BraveWallet.TransactionStatus
  sender: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
  value: string
  valueExact: string
  symbol: string
  decimals: number
  insufficientFundsForGasError?: boolean
  insufficientFundsError?: boolean
  contractAddressError?: string
  sameAddressError?: string
  erc721BlockchainToken?: BraveWallet.BlockchainToken
  erc721TokenId?: string
  isSwap?: boolean
  intent: string

  // Tx type flags
  isSolanaTransaction: boolean
  isSolanaDappTransaction: boolean
  isFilecoinTransaction: boolean

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
  isApprovalUnlimited?: boolean

  // Swap
  sellToken?: BraveWallet.BlockchainToken
  sellAmount?: Amount
  buyToken?: BraveWallet.BlockchainToken
  minBuyAmount?: Amount

  // Solana Dapp Instructions
  instructions?: TypedSolanaInstructionWithParams[]
}

export function useTransactionFeesParser (selectedNetwork?: BraveWallet.NetworkInfo, networkSpotPrice?: string, solFeeEstimates?: SolFeeEstimates) {
  /**
   * Checks if a given gasLimit is empty or zero-value, and returns an
   * appropriate localized error string.
   *
   * @remarks
   *
   * This function may only be used on ALL transaction types.
   *
   * @param gasLimit - The parsed gasLimit string.
   * @returns Localized string describing the error, or undefined in case of
   * no error.
   */
  const checkForMissingGasLimitError = React.useCallback((gasLimit: string): string | undefined => {
    return (gasLimit === '' || Amount.normalize(gasLimit) === '0')
      ? getLocale('braveWalletMissingGasLimitError')
      : undefined
  }, [])

  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransactionFees => {
    const { txDataUnion: { ethTxData1559: txData, filTxData } } = transactionInfo

    const isFilTransaction = filTxData !== undefined
    const isSolanaTxn = isSolanaTransaction(transactionInfo)

    const gasLimit = isFilTransaction
      ? filTxData.gasLimit
      : txData?.baseData.gasLimit || ''

    const gasPrice = txData?.baseData.gasPrice || ''
    const maxFeePerGas = txData?.maxFeePerGas || ''
    const maxPriorityFeePerGas = txData?.maxPriorityFeePerGas || ''
    const isEIP1559Transaction = maxPriorityFeePerGas !== '' && maxFeePerGas !== ''

    // [FIXME] - Extract actual fees used in the Solana transaction, instead of
    //   populating current estimates.
    const gasFee = isSolanaTxn
      ? new Amount(solFeeEstimates?.fee.toString() ?? '').format()
      : isEIP1559Transaction
        ? new Amount(maxFeePerGas)
          .times(gasLimit)
          .format()
        : new Amount(gasPrice)
          .times(gasLimit)
          .format()

    return {
      gasLimit: Amount.normalize(gasLimit),
      gasPrice: Amount.normalize(gasPrice),
      maxFeePerGas: Amount.normalize(maxFeePerGas),
      maxPriorityFeePerGas: Amount.normalize(maxPriorityFeePerGas),
      gasFee,
      gasFeeFiat: selectedNetwork && networkSpotPrice
        ? new Amount(gasFee)
          .divideByDecimals(selectedNetwork.decimals)
          .times(networkSpotPrice)
          .formatAsFiat()
        : '',
      isEIP1559Transaction,
      missingGasLimitError: isSolanaTxn
        ? undefined
        : checkForMissingGasLimitError(gasLimit),
      gasPremium: isFilTransaction ? new Amount(filTxData.gasPremium).format() : '',
      gasFeeCap: isFilTransaction ? new Amount(filTxData.gasFeeCap).format() : ''
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  transactionNetwork?: BraveWallet.NetworkInfo
) {
  const {
    selectedNetwork: reduxSelectedNetwork,
    fullTokenList,
    userVisibleTokensInfo: visibleTokens,
    accounts,
    transactionSpotPrices: spotPrices,
    solFeeEstimates
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const selectedNetwork = transactionNetwork || reduxSelectedNetwork
  const nativeAsset = React.useMemo(
    () => selectedNetwork && makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )
  const { findAssetPrice, computeFiatAmount } = usePricing(spotPrices)

  const networkSpotPrice = React.useMemo(
    () => selectedNetwork
      ? findAssetPrice(selectedNetwork.symbol)
      : '',
    [selectedNetwork, findAssetPrice]
  )
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, networkSpotPrice, solFeeEstimates)

  const findToken = React.useCallback((contractAddress: string) => {
    const checkVisibleList = visibleTokens.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase())
    return checkVisibleList ?? (fullTokenList?.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase()))
  }, [visibleTokens, fullTokenList])

  /**
   * Checks if a given address is a known contract address from our token
   * registry.
   *
   * @remarks
   *
   * This function must only be used for the following transaction types:
   *  - ERC20Transfer
   *  - ERC721TransferFrom
   *  - ERC721SafeTransferFrom
   *
   * @param to - The address to check
   * @returns Localized string describing the error, or undefined in case of
   * no error.
   */
  const checkForContractAddressError = (to: string): string | undefined => {
    return fullTokenList?.some(token => token.contractAddress.toLowerCase() === to.toLowerCase())
      ? getLocale('braveWalletContractAddressError')
      : undefined
  }

  /**
   * Checks if a given set of sender and recipient addresses are the
   * same.
   *
   * @remarks
   *
   * This function must only be used for the following transaction types:
   *  - ERC20Transfer
   *  - ERC721TransferFrom
   *  - ERC721SafeTransferFrom
   *  - ERC20Approve
   *  - ETHSend
   *
   * @param to - The recipient address
   * @param from - The sender address
   */
  const checkForSameAddressError = (to: string, from: string): string | undefined => {
    return to.toLowerCase() === from.toLowerCase()
      ? getLocale('braveWalletSameAddressError')
      : undefined
  }

  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransaction => {
    const {
      txArgs,
      txDataUnion: {
        ethTxData1559: txData,
        solanaTxData: solTxData,
        filTxData
      },
      fromAddress,
      txType
    } = transactionInfo

    const feeDetails = parseTransactionFees(transactionInfo)
    const { gasFeeFiat, gasFee } = feeDetails

    const isFilTransaction = isFilecoinTransaction(transactionInfo)
    const isSolanaTxn = isSolanaTransaction(transactionInfo)

    const isSPLTransaction =
      txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
      txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation

    const value =
      isSPLTransaction ? solTxData?.amount.toString() ?? ''
        : isSolanaTxn ? solTxData?.lamports.toString() ?? ''
          : isFilTransaction ? filTxData?.value || ''
            : txData?.baseData.value || ''

    const to = getTransactionToAddress(transactionInfo)
    const nonce = getTransactionNonce(transactionInfo)
    const account = accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())
    const token = isSPLTransaction
      ? findToken(solTxData?.splTokenMintAddress ?? '')
      : findToken(getTransactionInteractionAddress(transactionInfo))
    const accountNativeBalance = getBalance(account, nativeAsset)
    const accountTokenBalance = getBalance(account, token)

    const txBase: Pick<
      ParsedTransaction,
      | 'isFilecoinTransaction'
      | 'isSolanaDappTransaction'
      | 'isSolanaTransaction'
      | 'nonce'
      | 'recipient'
      | 'recipientLabel'
    > = {
      isFilecoinTransaction: isFilTransaction,
      isSolanaDappTransaction: isSolanaDappTransaction(transactionInfo),
      isSolanaTransaction: isSolanaTxn,
      nonce,
      recipient: to,
      recipientLabel: getAddressLabel(to, accounts)
    }

    switch (true) {
      case txBase.isSolanaDappTransaction: {
        const instructions = solTxData ? getTypedSolanaTxInstructions(solTxData) : []

        const lamportsMovedFromInstructions = instructions.reduce((acc, { type, params }) => {
          const lamportsAmount = (params as SolanaParamsWithLamports)?.lamports?.toString() ?? '0'

          switch (type) {
            case 'Transfer':
            case 'TransferWithSeed': {
              const { fromPubkey, toPubkey } = params as Solana.TransferParams | Solana.TransferWithSeedParams

              // only show lamports as transfered if the amount is going to a different pubKey
              if (!toPubkey.equals(fromPubkey)) {
                return acc.plus(lamportsAmount)
              }
              return acc
            }

            case 'WithdrawNonceAccount': {
              const { noncePubkey, toPubkey } = params as Solana.WithdrawNonceParams

              if (noncePubkey.equals(new Solana.PublicKey(fromAddress))) {
                return acc.plus(lamportsAmount)
              }

              if (toPubkey.equals(new Solana.PublicKey(fromAddress))) {
                return acc.minus(lamportsAmount)
              }

              return acc
            }

            case 'Create':
            case 'CreateWithSeed': {
              const { fromPubkey } = params as Solana.CreateAccountParams | Solana.CreateAccountWithSeedParams

              if (fromPubkey.toString() === fromAddress) {
                return acc.plus(lamportsAmount)
              }

              return acc
            }

            case 'Unknown':
            default: return acc.plus(lamportsAmount)
          }
        }, new Amount(0)) ?? 0

        const transferedValue = selectedNetwork
          ? new Amount(value)
            .divideByDecimals(selectedNetwork.decimals)
            .plus(lamportsMovedFromInstructions)
            .format()
          : ''

        const transferedAmountFiat = selectedNetwork
          ? computeFiatAmount(transferedValue, selectedNetwork.symbol, selectedNetwork.decimals)
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(transferedAmountFiat)

        const parsedTx: ParsedTransaction = {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress, accounts),
          recipient: to,
          recipientLabel: getAddressLabel(to, accounts),
          fiatValue: transferedAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: transferedAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: selectedNetwork
            ? new Amount(transferedValue)
              .divideByDecimals(selectedNetwork.decimals)
              .format(6)
            : '',
          valueExact: selectedNetwork
            ? new Amount(transferedValue)
              .divideByDecimals(selectedNetwork.decimals)
              .format()
            : '',
          symbol: selectedNetwork?.symbol ?? '',
          decimals: selectedNetwork?.decimals ?? 18,
          insufficientFundsError: accountNativeBalance !== ''
            ? new Amount(transferedValue)
              .plus(gasFee)
              .gt(accountNativeBalance)
            : undefined,
          insufficientFundsForGasError: accountNativeBalance !== ''
            ? new Amount(gasFee).gt(accountNativeBalance)
            : undefined,
          isSwap: txType === BraveWallet.TransactionType.SolanaSwap,
          instructions,
          intent: txType === BraveWallet.TransactionType.SolanaSwap
            ? getLocale('braveWalletSwap')
            : getLocale('braveWalletTransactionIntentDappInteraction'),
          ...feeDetails
        }

        return parsedTx
      }

      // transfer(address recipient, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Transfer: {
        const [address, amount] = txArgs
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(amount)
          .divideByDecimals(token?.decimals ?? 18)
          .times(price)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined
        const insufficientTokenFunds = accountTokenBalance !== ''
          ? new Amount(amount).gt(accountTokenBalance)
          : undefined

        const valueWrapped = new Amount(amount)
          .divideByDecimals(token?.decimals ?? 18)

        return {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress, accounts),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: valueWrapped.format(6),
          valueExact: valueWrapped.format(),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          insufficientFundsError: insufficientTokenFunds,
          insufficientFundsForGasError: insufficientNativeFunds,
          contractAddressError: checkForContractAddressError(address),
          sameAddressError: checkForSameAddressError(address, fromAddress),
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', valueWrapped.formatAsAsset(6, token?.symbol)),
          ...feeDetails
        } as ParsedTransaction
      }

      // transferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721TransferFrom:

      // safeTransferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721SafeTransferFrom: {
        // The owner of the ERC721 must not be confused with the
        // caller (fromAddress).
        const [owner, toAddress, tokenID] = txArgs

        const totalAmountFiat = gasFeeFiat

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        const erc721TokenId = tokenID && `#${Amount.normalize(tokenID)}`

        return {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress, // The caller, which may not be the owner
          senderLabel: getAddressLabel(fromAddress, accounts),
          fiatValue: Amount.zero(), // Display NFT values in the future
          fiatTotal: new Amount(totalAmountFiat),
          formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: '1', // Can only send 1 erc721 at a time
          valueExact: '1',
          symbol: token?.symbol ?? '',
          decimals: 0,
          insufficientFundsForGasError: insufficientNativeFunds,
          insufficientFundsError: false,
          erc721BlockchainToken: token,
          erc721TokenId,
          contractAddressError: checkForContractAddressError(toAddress),
          sameAddressError: checkForSameAddressError(toAddress, owner),
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', `${token?.symbol ?? ''} ${erc721TokenId}`),
          ...feeDetails
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Approve: {
        const [address, amount] = txArgs
        const totalAmountFiat = new Amount(gasFeeFiat)
        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        const amountWrapped = new Amount(amount)

        return {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress, accounts),
          fiatValue: Amount.zero(),
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: Amount.zero()
            .formatAsAsset(2, selectedNetwork?.symbol),
          value: amountWrapped
            .divideByDecimals(token?.decimals ?? 18)
            .format(6),
          valueExact: amountWrapped
            .divideByDecimals(token?.decimals ?? 18)
            .format(),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          approvalTarget: address,
          approvalTargetLabel: getAddressLabel(address, accounts),
          isApprovalUnlimited: amountWrapped.eq(MAX_UINT256),
          insufficientFundsForGasError: insufficientNativeFunds,
          insufficientFundsError: false,
          sameAddressError: checkForSameAddressError(address, fromAddress),
          intent: toProperCase(getLocale('braveWalletApprovalTransactionIntent')) + ' ' + token?.symbol ?? '',
          ...feeDetails
        } as ParsedTransaction
      }

      case txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer:
      case txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation: {
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(value)
          .divideByDecimals(token?.decimals ?? 9)
          .times(price)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined
        const insufficientTokenFunds = accountTokenBalance !== ''
          ? new Amount(value).gt(accountTokenBalance)
          : undefined

        const valueWrapped = new Amount(value)
          .divideByDecimals(token?.decimals ?? 9)

        return {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress, accounts),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: valueWrapped.format(6),
          valueExact: valueWrapped.format(),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 9,
          insufficientFundsError: insufficientTokenFunds,
          insufficientFundsForGasError: insufficientNativeFunds,
          contractAddressError: checkForContractAddressError(solTxData?.toWalletAddress ?? ''),
          sameAddressError: checkForSameAddressError(solTxData?.toWalletAddress ?? '', fromAddress),
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', valueWrapped.formatAsAsset(6, token?.symbol)),
          ...feeDetails
        } as ParsedTransaction
      }

      // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
      case txType === BraveWallet.TransactionType.ETHSwap: {
        const [fillPath, sellAmountArg, minBuyAmountArg] = txArgs
        const fillContracts = fillPath
          .slice(2)
          .match(/.{1,40}/g)
        const fillTokens = (fillContracts || [])
          .map(path => '0x' + path)
          .map(address =>
            address === NATIVE_ASSET_CONTRACT_ADDRESS_0X
              ? nativeAsset
              : findToken(address) || nativeAsset)
          .filter(Boolean) as BraveWallet.BlockchainToken[]

        const sellToken = fillTokens.length === 1
          ? nativeAsset
          : fillTokens[0]
        const sellAmountWeiBN = new Amount(sellAmountArg || value)
        const sellAmountFiat = sellToken
          ? computeFiatAmount(
              sellAmountWeiBN.format(),
              sellToken.symbol,
              sellToken.decimals
            )
          : Amount.empty()

        const buyToken = fillTokens[fillTokens.length - 1]
        const buyAmount = new Amount(minBuyAmountArg)
          .divideByDecimals(buyToken.decimals)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sellAmountFiat)

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        const sellTokenBalance = getBalance(account, sellToken)
        const insufficientTokenFunds = sellTokenBalance !== ''
          ? sellAmountWeiBN.gt(sellTokenBalance)
          : undefined

        const sellAmountBN = sellToken
          ? sellAmountWeiBN
            .divideByDecimals(sellToken.decimals)
          : Amount.empty()

        return {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress, accounts),
          fiatValue: sellAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sellAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: sellAmountBN.format(6),
          valueExact: sellAmountBN.format(),
          symbol: sellToken?.symbol ?? '',
          decimals: sellToken?.decimals ?? 18,
          insufficientFundsError: insufficientTokenFunds,
          insufficientFundsForGasError: insufficientNativeFunds,
          isSwap: true,
          sellToken,
          sellAmount: sellAmountBN,
          buyToken,
          minBuyAmount: buyAmount,
          intent: getLocale('braveWalletTransactionIntentSwap')
            .replace('$1', sellAmountBN.formatAsAsset(6, sellToken?.symbol))
            .replace('$2', buyAmount.formatAsAsset(6, buyToken.symbol)),
          ...feeDetails
        } as ParsedTransaction
      }

      case to.toLowerCase() === SwapExchangeProxy:
      case txType === BraveWallet.TransactionType.ETHSend:
      case txType === BraveWallet.TransactionType.SolanaSystemTransfer:
      case txType === BraveWallet.TransactionType.Other:
      default: {
        const sendAmountFiat = selectedNetwork
          ? computeFiatAmount(value, selectedNetwork.symbol, selectedNetwork.decimals)
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const valueWrapped = selectedNetwork
          ? new Amount(value).divideByDecimals(selectedNetwork.decimals)
          : Amount.empty()

        return {
          ...txBase,
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress, accounts),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: valueWrapped.format(6),
          valueExact: valueWrapped.format(),
          symbol: selectedNetwork?.symbol ?? '',
          decimals: selectedNetwork?.decimals ?? 18,
          insufficientFundsError: accountNativeBalance !== ''
            ? new Amount(value)
              .plus(gasFee)
              .gt(accountNativeBalance)
            : undefined,
          insufficientFundsForGasError: accountNativeBalance !== ''
            ? new Amount(gasFee).gt(accountNativeBalance)
            : undefined,
          isSwap: to.toLowerCase() === SwapExchangeProxy,
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', valueWrapped.formatAsAsset(6, selectedNetwork?.symbol)),
          ...feeDetails
        } as ParsedTransaction
      }
    }
  }, [
    selectedNetwork,
    accounts,
    spotPrices,
    findToken
  ])
}

export function parseTransactionWithoutPrices ({
  accounts,
  fullTokenList,
  tx,
  transactionNetwork,
  userVisibleTokensList,
  solFeeEstimates
}: {
  accounts: WalletAccountType[]
  fullTokenList: BraveWallet.BlockchainToken[]
  solFeeEstimates?: SolFeeEstimates
  tx: BraveWallet.TransactionInfo
  transactionNetwork: BraveWallet.NetworkInfo
  userVisibleTokensList: BraveWallet.BlockchainToken[]
}): ParsedTransaction {
  const to = getTransactionToAddress(tx)
  return {
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSolanaDappTransaction: isSolanaTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    nonce: getTransactionNonce(tx),
    recipient: to,
    recipientLabel: getAddressLabel(to, accounts)
  } as ParsedTransaction
}
