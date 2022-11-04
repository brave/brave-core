/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import {
  BraveWallet,
  SolFeeEstimates,
  TimeDelta,
  WalletAccountType,
  WalletState
} from '../../constants/types'
import { NATIVE_ASSET_CONTRACT_ADDRESS_0X } from '../constants/magics'
import { SwapExchangeProxy } from '../constants/registry'

// Utils
import { getLocale } from '../../../common/locale'
import Amount from '../../utils/amount'
import {
  getTypedSolanaTxInstructions,
  TypedSolanaInstructionWithParams
} from '../../utils/solana-instruction-utils'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  findTransactionAccount,
  findTransactionToken,
  getETHSwapTranasactionBuyAndSellTokens,
  getFormattedTransactionTransferredValue,
  getGasFeeFiatValue,
  getIsTxApprovalUnlimited,
  getTransactionApprovalTargetAddress,
  getTransactionBaseValue,
  getTransactionDecimals,
  getTransactionErc721TokenId,
  getTransactionIntent,
  getTransactionNonce,
  getTransactionToAddress,
  getTransactionTokenSymbol,
  isFilecoinTransaction,
  isSendingToKnownTokenContractAddress,
  isSolanaDappTransaction,
  isSolanaSplTransaction,
  isSolanaTransaction,
  isSwapTransaction,
  parseTransactionFeesWithoutPrices,
  transactionHasSameAddressError
} from '../../utils/tx-utils'
import { getBalance } from '../../utils/balance-utils'
import { getAddressLabel } from '../../utils/account-utils'
import { makeNetworkAsset } from '../../options/asset-options'
import { findTokenByContractAddress } from '../../utils/asset-utils'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'

// Hooks
import usePricing from './pricing'

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
  id: string
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
  decimals: number // network decimals
  insufficientFundsForGasError?: boolean
  insufficientFundsError?: boolean
  contractAddressError?: string
  sameAddressError?: string
  erc721TokenId?: string
  isSwap?: boolean
  intent: string

  // Tx type flags
  isSolanaTransaction: boolean
  isSolanaDappTransaction: boolean
  isSolanaSPLTransaction: boolean
  isFilecoinTransaction: boolean
  coinType: BraveWallet.CoinType

  // Tokens
  token?: BraveWallet.BlockchainToken
  erc721BlockchainToken?: BraveWallet.BlockchainToken

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
  isApprovalUnlimited?: boolean

  // Swap
  sellToken?: BraveWallet.BlockchainToken
  sellAmount?: Amount
  sellAmountWei?: Amount
  buyToken?: BraveWallet.BlockchainToken
  minBuyAmount?: Amount
  minBuyAmountWei?: Amount

  // Solana Dapp Instructions
  instructions?: TypedSolanaInstructionWithParams[]
}

export function useTransactionFeesParser (selectedNetwork?: BraveWallet.NetworkInfo, networkSpotPrice?: string, solFeeEstimates?: SolFeeEstimates) {
  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransactionFees => {
    const txFeesBase = parseTransactionFeesWithoutPrices(
      transactionInfo,
      solFeeEstimates
    )

    return {
      ...txFeesBase,
      gasFeeFiat: getGasFeeFiatValue({
        gasFee: txFeesBase.gasFee,
        networkSpotPrice,
        txNetwork: selectedNetwork
      }),
      missingGasLimitError: txFeesBase.isMissingGasLimit
        ? getLocale('braveWalletMissingGasLimitError')
        : undefined
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

  const combinedTokensList = React.useMemo(() => {
    return visibleTokens.concat(fullTokenList)
  }, [visibleTokens, fullTokenList])

  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransaction => {
    const { txArgs, txType } = transactionInfo

    const {
      gasFee,
      gasFeeCap,
      gasLimit,
      gasPremium,
      gasPrice,
      isMissingGasLimit,
      maxFeePerGas,
      isEIP1559Transaction,
      maxPriorityFeePerGas
    } = parseTransactionFeesWithoutPrices(transactionInfo, solFeeEstimates)
    const gasFeeFiat = getGasFeeFiatValue({
      gasFee,
      networkSpotPrice,
      txNetwork: transactionNetwork
    })

    const isFilTransaction = isFilecoinTransaction(transactionInfo)
    const isSolanaTxn = isSolanaTransaction(transactionInfo)
    const isSPLTransaction = isSolanaSplTransaction(transactionInfo)
    const baseValue = getTransactionBaseValue(transactionInfo)
    const to = getTransactionToAddress(transactionInfo)
    const nonce = getTransactionNonce(transactionInfo)
    const account = findTransactionAccount(accounts, transactionInfo)
    const token = findTransactionToken(transactionInfo, combinedTokensList)
    const accountNativeBalance = getBalance(account, nativeAsset)
    const accountTokenBalance = getBalance(account, token)

    const {
      buyToken,
      sellToken,
      buyAmount,
      sellAmount,
      sellAmountWei,
      buyAmountWei
    } = getETHSwapTranasactionBuyAndSellTokens({
      nativeAsset,
      tokensList: combinedTokensList,
      tx: transactionInfo
    })

    const {
      normalizedTransferredValue,
      normalizedTransferredValueExact,
      weiTransferredValue
    } = getFormattedTransactionTransferredValue({
      tx: transactionInfo,
      txNetwork: transactionNetwork,
      token,
      sellToken
    })

    const erc721Token = [
      BraveWallet.TransactionType.ERC721TransferFrom,
      BraveWallet.TransactionType.ERC721SafeTransferFrom
    ].includes(transactionInfo.txType) ? token : undefined

    const erc721TokenId = getTransactionErc721TokenId(transactionInfo)
    const approvalTarget = getTransactionApprovalTargetAddress(transactionInfo)

    const insufficientFundsError = accountHasInsufficientFundsForTransaction({
      accountNativeBalance,
      accountTokenBalance,
      gasFee,
      tokensList: combinedTokensList,
      tx: transactionInfo,
      account,
      nativeAsset
    })

    const txBase: Pick<
      ParsedTransaction,
      | 'approvalTarget'
      | 'approvalTargetLabel'
      | 'buyToken'
      | 'coinType'
      | 'contractAddressError'
      | 'createdTime'
      | 'decimals'
      | 'erc721BlockchainToken'
      | 'erc721TokenId'
      | 'gasFee'
      | 'gasFeeCap'
      | 'gasFeeFiat'
      | 'gasLimit'
      | 'missingGasLimitError'
      | 'gasPremium'
      | 'gasPrice'
      | 'hash'
      | 'id'
      | 'isEIP1559Transaction'
      | 'instructions'
      | 'insufficientFundsError'
      | 'insufficientFundsForGasError'
      | 'intent'
      | 'isApprovalUnlimited'
      | 'isFilecoinTransaction'
      | 'isSolanaDappTransaction'
      | 'isSolanaSPLTransaction'
      | 'isSolanaTransaction'
      | 'isSwap'
      | 'maxFeePerGas'
      | 'maxPriorityFeePerGas'
      | 'minBuyAmount'
      | 'minBuyAmountWei'
      | 'nonce'
      | 'recipient'
      | 'recipientLabel'
      | 'sameAddressError'
      | 'sellAmount'
      | 'sellAmountWei'
      | 'sellToken'
      | 'sender'
      | 'senderLabel'
      | 'status'
      | 'symbol'
      | 'token'
      | 'value'
      | 'valueExact'
    > = {
      approvalTarget,
      approvalTargetLabel: getAddressLabel(approvalTarget, accounts),
      buyToken,
      coinType: getCoinFromTxDataUnion(transactionInfo.txDataUnion),
      createdTime: transactionInfo.createdTime,
      contractAddressError: isSendingToKnownTokenContractAddress(transactionInfo, combinedTokensList)
        ? getLocale('braveWalletContractAddressError')
        : undefined,
      decimals: getTransactionDecimals({
        tx: transactionInfo,
        network: transactionNetwork,
        sellToken,
        erc721Token,
        token
      }),
      erc721BlockchainToken: erc721Token,
      erc721TokenId,
      gasFeeFiat,
      gasFee,
      gasFeeCap,
      gasLimit,
      gasPremium,
      gasPrice,
      maxFeePerGas,
      maxPriorityFeePerGas,
      missingGasLimitError: isMissingGasLimit
        ? getLocale('braveWalletMissingGasLimitError')
        : undefined,
      hash: transactionInfo.txHash,
      id: transactionInfo.id,
      isEIP1559Transaction,
      instructions: getTypedSolanaTxInstructions(transactionInfo.txDataUnion.solanaTxData),
      insufficientFundsError,
      insufficientFundsForGasError: accountHasInsufficientFundsForGas({
        accountNativeBalance,
        gasFee
      }),
      intent: getTransactionIntent({
        normalizedTransferredValue,
        tx: transactionInfo,
        buyAmount,
        buyToken,
        erc721TokenId,
        sellAmount,
        sellToken,
        token,
        transactionNetwork
      }),
      isApprovalUnlimited: getIsTxApprovalUnlimited(transactionInfo),
      isFilecoinTransaction: isFilTransaction,
      isSolanaDappTransaction: isSolanaDappTransaction(transactionInfo),
      isSolanaSPLTransaction: isSPLTransaction,
      isSolanaTransaction: isSolanaTxn,
      isSwap: isSwapTransaction(transactionInfo),
      minBuyAmount: buyAmount,
      minBuyAmountWei: buyAmountWei,
      nonce,
      recipient: to,
      recipientLabel: getAddressLabel(to, accounts),
      sameAddressError: transactionHasSameAddressError(transactionInfo) ? getLocale('braveWalletSameAddressError') : undefined,
      sellAmount,
      sellAmountWei,
      sellToken,
      sender: transactionInfo.fromAddress,
      senderLabel: getAddressLabel(transactionInfo.fromAddress, accounts),
      status: transactionInfo.txStatus,
      symbol: getTransactionTokenSymbol({
        tx: transactionInfo,
        txNetwork: transactionNetwork,
        token,
        sellToken
      }),
      token,
      value: normalizedTransferredValue,
      valueExact: normalizedTransferredValueExact
    }

    switch (true) {
      case txBase.isSolanaDappTransaction: {
        const transferedAmountFiat = selectedNetwork
          ? computeFiatAmount(
              normalizedTransferredValueExact,
              selectedNetwork.symbol,
              selectedNetwork.decimals
            )
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(transferedAmountFiat)

        const parsedTx: ParsedTransaction = {
          ...txBase,
          recipient: to,
          recipientLabel: getAddressLabel(to, accounts),
          fiatValue: transferedAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: transferedAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol)
        }

        return parsedTx
      }

      // transfer(address recipient, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Transfer: {
        const [, amount] = txArgs
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(amount)
          .divideByDecimals(token?.decimals ?? 18)
          .times(price)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        return {
          ...txBase,
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol)
        } as ParsedTransaction
      }

      // transferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721TransferFrom:

      // safeTransferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721SafeTransferFrom: {
        const totalAmountFiat = gasFeeFiat

        return {
          ...txBase,
          fiatValue: Amount.zero(), // Display NFT values in the future
          fiatTotal: new Amount(totalAmountFiat),
          formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol)
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Approve: {
        const totalAmountFiat = new Amount(gasFeeFiat)

        return {
          ...txBase,
          fiatValue: Amount.zero(),
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: Amount.zero()
            .formatAsAsset(2, selectedNetwork?.symbol)
        } as ParsedTransaction
      }

      case isSPLTransaction: {
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(normalizedTransferredValue)
          .times(price)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        return {
          ...txBase,
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: normalizedTransferredValue,
          valueExact: normalizedTransferredValueExact
        } as ParsedTransaction
      }

      // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
      case txType === BraveWallet.TransactionType.ETHSwap: {
        const [fillPath] = txArgs

        const fillContracts = fillPath
          .slice(2)
          .match(/.{1,40}/g)
        const fillTokens = (fillContracts || [])
          .map(path => '0x' + path)
          .map(address =>
            address === NATIVE_ASSET_CONTRACT_ADDRESS_0X
              ? nativeAsset
              : findTokenByContractAddress(address, combinedTokensList) || nativeAsset)
          .filter(Boolean) as BraveWallet.BlockchainToken[]

        const sellToken = fillTokens.length === 1
          ? nativeAsset
          : fillTokens[0]

        const sellAmountFiat = sellToken
          ? computeFiatAmount(
              weiTransferredValue,
              sellToken.symbol,
              sellToken.decimals
            )
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sellAmountFiat)

        return {
          ...txBase,
          fiatValue: sellAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sellAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol)
        } as ParsedTransaction
      }

      case to.toLowerCase() === SwapExchangeProxy:
      case txType === BraveWallet.TransactionType.ETHSend:
      case txType === BraveWallet.TransactionType.SolanaSystemTransfer:
      case txType === BraveWallet.TransactionType.Other:
      default: {
        const sendAmountFiat = selectedNetwork
          ? computeFiatAmount(baseValue, selectedNetwork.symbol, selectedNetwork.decimals)
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        return {
          ...txBase,
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: normalizedTransferredValue,
          valueExact: normalizedTransferredValueExact
        } as ParsedTransaction
      }
    }
  }, [
    selectedNetwork,
    accounts,
    spotPrices
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
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const token = findTransactionToken(tx, combinedTokensList)
  const nativeAsset = makeNetworkAsset(transactionNetwork)
  const account = findTransactionAccount(accounts, tx)
  const accountNativeBalance = getBalance(account, nativeAsset)
  const accountTokenBalance = getBalance(account, token)

  const {
    buyToken,
    sellToken,
    buyAmount,
    sellAmount,
    sellAmountWei,
    buyAmountWei
  } = getETHSwapTranasactionBuyAndSellTokens({
    nativeAsset,
    tokensList: combinedTokensList,
    tx
  })

  const {
    normalizedTransferredValue,
    normalizedTransferredValueExact
  } = getFormattedTransactionTransferredValue({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const erc721Token = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType) ? token : undefined

  const approvalTarget = getTransactionApprovalTargetAddress(tx)

  const {
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isMissingGasLimit,
    maxFeePerGas,
    maxPriorityFeePerGas,
    isEIP1559Transaction
  } = parseTransactionFeesWithoutPrices(tx, solFeeEstimates)

  const insufficientFundsError = accountHasInsufficientFundsForTransaction({
    accountNativeBalance,
    accountTokenBalance,
    gasFee,
    tokensList: combinedTokensList,
    tx,
    account,
    nativeAsset
  })

  const erc721TokenId = getTransactionErc721TokenId(tx)

  return {
    gasFee,
    gasLimit,
    gasPrice,
    gasFeeCap,
    gasPremium,
    maxFeePerGas,
    maxPriorityFeePerGas,
    missingGasLimitError: isMissingGasLimit
      ? getLocale('braveWalletMissingGasLimitError')
      : undefined,
    approvalTarget,
    approvalTargetLabel: getAddressLabel(approvalTarget, accounts),
    buyToken,
    coinType: getCoinFromTxDataUnion(tx.txDataUnion),
    createdTime: tx.createdTime,
    contractAddressError: isSendingToKnownTokenContractAddress(tx, combinedTokensList)
      ? getLocale('braveWalletContractAddressError')
      : undefined,
    decimals: getTransactionDecimals({
      tx,
      network: transactionNetwork,
      sellToken,
      erc721Token,
      token
    }),
    erc721BlockchainToken: erc721Token,
    erc721TokenId,
    hash: tx.txHash,
    id: tx.id,
    instructions: getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData),
    insufficientFundsError,
    insufficientFundsForGasError: accountHasInsufficientFundsForGas({
      accountNativeBalance,
      gasFee
    }),
    intent: getTransactionIntent({
      normalizedTransferredValue,
      tx,
      buyAmount,
      buyToken,
      erc721TokenId,
      sellAmount,
      sellToken,
      token,
      transactionNetwork
    }),
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    isEIP1559Transaction,
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSolanaDappTransaction: isSolanaTransaction(tx),
    isSolanaSPLTransaction: isSolanaSplTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSwap: isSwapTransaction(tx),
    minBuyAmount: buyAmount,
    minBuyAmountWei: buyAmountWei,
    nonce: getTransactionNonce(tx),
    recipient: to,
    recipientLabel: getAddressLabel(to, accounts),
    sameAddressError: transactionHasSameAddressError(tx)
      ? getLocale('braveWalletSameAddressError')
      : undefined,
    sellAmount,
    sellAmountWei,
    sellToken,
    sender: tx.fromAddress,
    senderLabel: getAddressLabel(tx.fromAddress, accounts),
    status: tx.txStatus,
    symbol: getTransactionTokenSymbol({
      tx,
      txNetwork: transactionNetwork,
      token,
      sellToken
    }),
    token,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact
  } as ParsedTransaction
}
