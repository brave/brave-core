// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useParams } from 'react-router'

import ProgressRing from '@brave/leo/react/progressRing'

// Hooks
import {
  useGetMainnetsQuery,
  useGetTopDappsQuery
} from '../../../../common/slices/api.slice'

// Utils
import Amount from '../../../../utils/amount'
import { CurrencySymbols } from '../../../../utils/currency-symbols'
import { getLocale } from '../../../../../common/locale'
import { WalletRoutes } from '../../../../constants/types'
import { openTab } from '../../../../utils/routes-utils'

// Components
import { NavButton } from '../../../extension/buttons'
import { CreateNetworkIcon } from '../../../shared/create-network-icon'

// Styles
import { Column, Row, Text } from '../../../shared/style'
import {
  DappCategoryLabel,
  DappMetricContainer
} from './web3_dapp_details.styles'

export const DappDetails = () => {
  // routing
  const { dappId } = useParams<{
    dappId: string
  }>()

  // queries
  const { isLoading, data: dapp } = useGetTopDappsQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      data: dappId
        ? res.data?.find(({ id }) => id === parseInt(dappId))
        : undefined
    })
  })

  const { data: networks } = useGetMainnetsQuery()

  // render
  if (isLoading) {
    return (
      <Column
        fullHeight
        fullWidth
      >
        <ProgressRing />
      </Column>
    )
  }

  // Dapp not found, redirect to the list
  if (!dapp) {
    return <Redirect to={WalletRoutes.Web3} />
  }

  return (
    <Column
      fullHeight
      fullWidth
      justifyContent='flex-start'
    >
      <Column
        gap={'8px'}
        fullWidth
        justifyContent='flex-start'
        alignItems='center'
        margin={'32px 0px 0px 0px'}
      >
        <img
          src={`chrome://image?${dapp.logo}`}
          width={72}
          height={72}
        />

        <Text>{dapp.name}</Text>

        <Row gap={'8px'}>
          {dapp.categories.map((cat) => (
            <DappCategoryLabel key={cat}>{cat}</DappCategoryLabel>
          ))}
        </Row>

        <Text textSize='14px'>{dapp.description}</Text>
      </Column>

      <Column
        fullWidth
        gap={'8px'}
        margin='24px 0px'
      >
        <Row gap={'8px'}>
          <DappMetric
            metricName={
              // TODO
              'Active wallets'
            }
            meticValue={dapp.uaw}
            isUSDValue={false}
          />
          <DappMetric
            metricName={getLocale('braveWalletTransactions')}
            meticValue={dapp.transactions}
            isUSDValue={false}
          />
        </Row>
        <Row gap={'8px'}>
          <DappMetric
            metricName={getLocale('braveWalletMarketDataVolumeColumn')}
            meticValue={dapp.volume}
            isUSDValue={true}
          />
          <DappMetric
            metricName={getLocale('braveWalletBalance')}
            meticValue={dapp.balance}
            isUSDValue={true}
          />
        </Row>
      </Column>

      <Column
        fullWidth
        alignItems={'flex-start'}
        justifyContent={'flex-start'}
        gap={'8px'}
        margin='0px 0px 20px 0px'
      >
        <Text textAlign='left'>{getLocale('braveWalletNetworks')}</Text>
        <Row
          justifyContent={'flex-start'}
          alignItems={'flex-start'}
        >
          {dapp.chains.map((chainName) => {
            const net = networks.find((n) =>
              n.chainName.toLowerCase().startsWith(
                // The dapps list uses '-' to separate words
                chainName.replaceAll('-', ' ')
              )
            )
            return net ? (
              <CreateNetworkIcon
                key={net.chainId}
                network={net}
                size='big'
                marginRight={8}
              />
            ) : null
          })}
        </Row>
      </Column>

      <NavButton
        minWidth='100%'
        isExternalLink
        buttonType='primary'
        onSubmit={() => {
          openTab(dapp.website)
        }}
        text={
          // TODO
          'Visit $1'.replace('$1', dapp.name)
        }
      />
    </Column>
  )
}

function DappMetric({
  meticValue,
  metricName,
  isUSDValue
}: {
  metricName: string
  meticValue: string | number
  isUSDValue: boolean
}) {
  return (
    <DappMetricContainer>
      <Text
        textAlign='left'
        textSize='12px'
      >
        {metricName}
      </Text>
      <Text
        textAlign='left'
        textSize='16px'
        isBold
      >
        {isUSDValue ? CurrencySymbols.USD : ''}
        {new Amount(meticValue).abbreviate(2)}
      </Text>
    </DappMetricContainer>
  )
}
