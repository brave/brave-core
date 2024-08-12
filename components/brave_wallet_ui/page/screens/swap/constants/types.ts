// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../../../constants/types'

import Amount from '../../../../utils/amount'

export type LiquiditySource = {
  name: string
  proportion: Amount
  includedSteps?: BraveWallet.LiFiStep[]
  tool?: string
  logo?: string
}

export type RouteTagsType = 'CHEAPEST' | 'FASTEST'

export type QuoteOption = {
  fromAmount: Amount
  toAmount: Amount
  minimumToAmount: Amount | undefined
  fromToken: BraveWallet.BlockchainToken
  toToken: BraveWallet.BlockchainToken
  rate: Amount
  impact: Amount
  sources: LiquiditySource[]

  /**
   * Indicates the kind of routing followed by the order:
   *
   * split -> indicates that the order was fulfilled from two separate LPs
   *
   * flow  -> indicates that the order was fulfilled through an intermediate
   *          asset between two separate LPs.
   */
  routing: 'split' | 'flow'

  networkFee: Amount

  networkFeeFiat: string
  provider: BraveWallet.SwapProvider
  executionDuration?: string
  tags: RouteTagsType[]
  id?: string
}

export type SwapAndSend = {
  label: string
  name: string
}

export type Exchange = {
  id: string
  name: string
}

export type GasEstimate = {
  gasFee: string
  gasFeeGwei?: string
  gasFeeFiat?: string
  time?: string
}

export type AmountValidationErrorType =
  | 'fromAmountDecimalsOverflow'
  | 'toAmountDecimalsOverflow'

export type SwapValidationErrorType =
  | AmountValidationErrorType
  | 'insufficientBalance'
  | 'insufficientFundsForGas'
  | 'insufficientAllowance'
  | 'insufficientLiquidity'
  | 'providerNotSupported'
  | 'unknownError'

export type SwapParams = {
  fromAccount?: BraveWallet.AccountInfo
  fromToken?: BraveWallet.BlockchainToken
  fromNetwork?: BraveWallet.NetworkInfo
  fromAmount: string

  toAccountId?: BraveWallet.AccountId
  toToken?: BraveWallet.BlockchainToken
  toAmount: string

  /**
   * This is the value as seen on the UI - should be converted to appropriate
   * format for Jupiter and 0x swap providers.
   */
  slippageTolerance: string
}

export type SwapParamsOverrides = {
  fromAmount?: string
  toAmount?: string
  fromToken?: BraveWallet.BlockchainToken
  toToken?: BraveWallet.BlockchainToken
  provider?: BraveWallet.SwapProvider
  slippage?: string
  selectedQuoteOptionId?: string
}
