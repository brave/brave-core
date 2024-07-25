// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import { getLPIcon } from '../../../swap.utils'

// Queries
import { useGetNetworkQuery } from '../../../../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import Amount from '../../../../../../utils/amount'

// Styles
import { LPIconWrapper, ArrowIcon } from './routes.style'
import { LPIcon, BankIcon } from '../../shared-swap.styles'
import { Row, Text, Column } from '../../../../../../components/shared/style'

interface Props {
  step: BraveWallet.LiFiStep
}

export const RouteStep = (props: Props) => {
  // Props
  const { step } = props
  const {
    action: { fromToken, toToken },
    toolDetails
  } = step

  // Computed
  const lpIcon = getLPIcon(toolDetails)

  // Queries
  const { data: fromNetwork } = useGetNetworkQuery(
    fromToken
      ? {
          chainId: fromToken.chainId,
          coin: fromToken.coin
        }
      : skipToken
  )

  const { data: toNetwork } = useGetNetworkQuery(
    toToken
      ? {
          chainId: toToken.chainId,
          coin: toToken.coin
        }
      : skipToken
  )

  return (
    <Row
      key={step.id}
      alignItems='flex-start'
      justifyContent='flex-start'
      margin='24px 0px 0px 3px'
    >
      <LPIconWrapper
        padding='2px'
        margin='0px 12px 0px 0px'
      >
        {lpIcon !== '' ? (
          <LPIcon
            size='14px'
            icon={lpIcon}
          />
        ) : (
          <BankIcon size='14px' />
        )}
      </LPIconWrapper>
      <Column
        padding='3px 0px 0px 0px'
        justifyContent='flex-start'
        alignItems='flex-start'
      >
        <Text
          textSize='12px'
          isBold={false}
          textColor='tertiary'
          textAlign='left'
        >
          {step.type === BraveWallet.LiFiStepType.kSwap
            ? getLocale('braveWalletSwapOnNetworkViaExchange')
                .replace('$1', fromNetwork?.chainName ?? '')
                .replace('$2', toolDetails.name)
            : getLocale('braveWalletBridgeFromNetworkToNetwork')
                .replace('$1', fromNetwork?.chainName ?? '')
                .replace('$2', toNetwork?.chainName ?? '')}
        </Text>
        <Row
          width='unset'
          justifyContent='flex-start'
        >
          <Text
            textSize='12px'
            isBold={false}
            textColor='secondary'
            textAlign='left'
          >
            {new Amount(step.estimate.fromAmount)
              .divideByDecimals(fromToken.decimals)
              .formatAsAsset(6, fromToken.symbol)}
          </Text>
          <ArrowIcon />
          <Text
            textSize='12px'
            isBold={false}
            textColor='secondary'
          >
            {new Amount(step.estimate.toAmount)
              .divideByDecimals(toToken.decimals)
              .formatAsAsset(6, toToken.symbol)}
          </Text>
        </Row>
      </Column>
    </Row>
  )
}
