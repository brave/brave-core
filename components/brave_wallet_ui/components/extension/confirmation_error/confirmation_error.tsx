// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

// Hooks
import {
  useFindBuySupportedToken, //
} from '../../../common/hooks/use-multi-chain-buy-assets'

// Types
import { BraveWallet } from '../../../constants/types'
import { ParsedTransaction } from '../../../utils/tx-utils'

// Utils
import { getLocale } from '../../../../common/locale'
import { makeNetworkAsset } from '../../../options/asset-options'
import {
  makeFundWalletRoute,
  openWalletRouteTab,
} from '../../../utils/routes-utils'

// Styled Components
import { Wrapper, ErrorText } from './confirmation_error.style'
import { Row } from '../../shared/style'

export interface Props {
  insufficientFundsError?: boolean
  insufficientFundsForGasError?: boolean
  transactionDetails?: ParsedTransaction
  transactionsNetwork?: BraveWallet.NetworkInfo
  account?: BraveWallet.AccountInfo
}

export const ConfirmationError = (props: Props) => {
  const {
    insufficientFundsError,
    insufficientFundsForGasError,
    transactionDetails,
    transactionsNetwork,
    account,
  } = props

  // memos
  const errors = React.useMemo(() => {
    return [
      transactionDetails?.contractAddressError,
      transactionDetails?.sameAddressError,
      transactionDetails?.missingGasLimitError,
      insufficientFundsForGasError
        ? getLocale('braveWalletSwapInsufficientFundsForGas')
        : undefined,
      !insufficientFundsForGasError && insufficientFundsError
        ? getLocale('braveWalletSwapInsufficientBalance')
        : undefined,
    ].filter((error): error is string => Boolean(error))
  }, [transactionDetails, insufficientFundsForGasError, insufficientFundsError])

  // Computed
  const nativeAsset = makeNetworkAsset(transactionsNetwork)
  const hasErrors = Boolean(errors.length)

  // Hooks
  const { foundMeldBuyToken } = useFindBuySupportedToken(nativeAsset)

  // Methods
  const onClickBuy = React.useCallback(() => {
    if (foundMeldBuyToken) {
      openWalletRouteTab(makeFundWalletRoute(foundMeldBuyToken, account))
    }
  }, [foundMeldBuyToken, account])

  if (!hasErrors) {
    return null
  }

  return errors.map((error) => (
    <Wrapper
      padding='8px 16px'
      justifyContent='space-between'
      key={error}
    >
      <Row
        width='unset'
        gap='8px'
      >
        <Icon name='warning-triangle-filled' />
        <ErrorText textColor='error'>{error}</ErrorText>
      </Row>
      {(insufficientFundsForGasError || insufficientFundsError)
        && foundMeldBuyToken && (
          <div>
            <Button
              kind='plain'
              size='tiny'
              onClick={onClickBuy}
            >
              {getLocale('braveWalletBuyAsset').replace(
                '$1',
                nativeAsset?.symbol ?? '',
              )}
            </Button>
          </div>
        )}
    </Wrapper>
  ))
}
