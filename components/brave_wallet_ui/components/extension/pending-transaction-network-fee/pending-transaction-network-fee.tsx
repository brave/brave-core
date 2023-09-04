// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// components
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import LoadingSkeleton from '../../shared/loading-skeleton'
import { EditButton } from '../confirm-transaction-panel/style'

// hooks
import {
  usePendingTransactions //
} from '../../../common/hooks/use-pending-transaction'
import {
  useGetDefaultFiatCurrencyQuery //
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
}

export const PendingTransactionNetworkFeeAndSettings: React.FC<Props> = ({
  onToggleAdvancedTransactionSettings,
  onToggleEditGas
}) => {
  // custom hooks
  const { transactionDetails, transactionsNetwork } = usePendingTransactions()

  // queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  return (
    <NetworkFeeAndSettingsContainer>
      <NetworkFeeContainer>
        <NetworkFeeTitle>{getLocale('braveWalletNetworkFees')}</NetworkFeeTitle>
        <NetworkFeeValue>
          <CreateNetworkIcon network={transactionsNetwork} marginRight={0} />
          {transactionDetails?.gasFeeFiat ? (
            new Amount(transactionDetails.gasFeeFiat).formatAsFiat(
              defaultFiatCurrency
            )
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
