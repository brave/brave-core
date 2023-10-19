// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import {
  getGasFeeFiatValue,
  getTransactionGasFee,
  isSolanaTransaction
} from '../../../utils/tx-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { makeNetworkAsset } from '../../../options/asset-options'

// components
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import LoadingSkeleton from '../../shared/loading-skeleton'
import { EditButton } from '../confirm-transaction-panel/style'

// hooks
import {
  useSelectedPendingTransaction //
} from '../../../common/hooks/use-pending-transaction'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'

// style
import {
  NetworkFeeAndSettingsContainer,
  NetworkFeeContainer,
  NetworkFeeTitle,
  NetworkFeeValue,
  Settings,
  SettingsIcon
} from './pending-transaction-network-fee.style'

interface Props {
  onToggleEditGas?: () => void
  onToggleAdvancedTransactionSettings?: () => void
  feeDisplayMode?: 'fiat' | 'crypto'
  showNetworkLogo?: boolean
}

export const PendingTransactionNetworkFeeAndSettings: React.FC<Props> = ({
  onToggleAdvancedTransactionSettings,
  onToggleEditGas,
  feeDisplayMode = 'fiat',
  showNetworkLogo
}) => {
  // custom hooks
  const selectedPendingTransaction = useSelectedPendingTransaction()

  // queries
  const { data: txNetwork } = useGetNetworkQuery(
    selectedPendingTransaction
      ? {
          chainId: selectedPendingTransaction.chainId,
          coin: getCoinFromTxDataUnion(selectedPendingTransaction.txDataUnion)
        }
      : skipToken
  )

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery(
    feeDisplayMode === 'fiat' ? undefined : skipToken
  )

  const networkTokenPriceId = txNetwork
    ? getPriceIdForToken(makeNetworkAsset(txNetwork))
    : ''

  const { data: pricesRegistry } = useGetTokenSpotPricesQuery(
    txNetwork && defaultFiatCurrency
      ? {
          ids: [networkTokenPriceId],
          toCurrency: defaultFiatCurrency,
          timeframe: BraveWallet.AssetPriceTimeframe.Live
        }
      : skipToken
  )

  const isSolTx = isSolanaTransaction(selectedPendingTransaction)

  const { data: solFeeEstimate = '' } = useGetSolanaEstimatedFeeQuery(
    selectedPendingTransaction && isSolTx
      ? {
          chainId: selectedPendingTransaction?.chainId || '',
          txId: selectedPendingTransaction?.id || ''
        }
      : skipToken
  )

  // computed
  const networkFee = selectedPendingTransaction
    ? isSolTx
      ? solFeeEstimate
      : getTransactionGasFee(selectedPendingTransaction)
    : ''

  const networkFeeFiat = getGasFeeFiatValue({
    gasFee: networkFee,
    networkSpotPrice: pricesRegistry?.[networkTokenPriceId]?.price || '',
    txNetwork
  })

  // render
  return (
    <NetworkFeeAndSettingsContainer>
      <NetworkFeeContainer>
        <NetworkFeeTitle>{getLocale('braveWalletNetworkFees')}</NetworkFeeTitle>
        <NetworkFeeValue>
          {showNetworkLogo ? (
            <CreateNetworkIcon
              network={txNetwork}
              marginRight={0}
            />
          ) : null}
          {feeDisplayMode === 'fiat' ? (
            networkFeeFiat ? (
              new Amount(networkFeeFiat).formatAsFiat(defaultFiatCurrency)
            ) : (
              <LoadingSkeleton width={38} />
            )
          ) : txNetwork ? (
            new Amount(networkFee)
              .divideByDecimals(txNetwork.decimals)
              .formatAsAsset(6, txNetwork?.symbol)
          ) : (
            <LoadingSkeleton width={38} />
          )}
          <EditButton onClick={onToggleEditGas}>
            {getLocale('braveWalletAllowSpendEditButton')}
          </EditButton>
        </NetworkFeeValue>
      </NetworkFeeContainer>

      {onToggleAdvancedTransactionSettings && (
        <Settings onClick={onToggleAdvancedTransactionSettings}>
          <SettingsIcon />
        </Settings>
      )}
    </NetworkFeeAndSettingsContainer>
  )
}
