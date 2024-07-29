// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { DialogProps } from '@brave/leo/react/dialog'

// Hooks
import { useGetMainnetsQuery } from '../../../../common/slices/api.slice'

// Utils
import Amount from '../../../../utils/amount'
import { CurrencySymbols } from '../../../../utils/currency-symbols'
import { getLocale } from '../../../../../common/locale'
import type { BraveWallet } from '../../../../constants/types'
import { isHttpsUrl } from '../../../../utils/string-utils'

// Components
import { CreateNetworkIcon } from '../../../shared/create-network-icon'
import { ExternalLink } from '../../../shared/external_link/external_link'

// Styles
import { Column, Row, Text } from '../../../shared/style'
import {
  DappDetailDialog,
  DappCategoryLabel,
  DappMetricContainer,
  Title
} from './web3_dapp_details.styles'
import { PlaceholderImage } from './dapp_list_item.styles'

interface DappDetailsProps extends DialogProps {
  dapp: BraveWallet.Dapp
}
export const DappDetails = ({ dapp, ...rest }: DappDetailsProps) => {
  // queries
  const { data: networks } = useGetMainnetsQuery()

  return (
    <DappDetailDialog
      {...rest}
      showClose={true}
    >
      <Title slot='title'>{getLocale('braveWalletDetails')}</Title>
      <Column
        fullHeight
        fullWidth
        justifyContent='flex-start'
        slot='default'
      >
        <Column
          gap={'8px'}
          fullWidth
          justifyContent='flex-start'
          alignItems='center'
          margin={'32px 0px 0px 0px'}
        >
          {isHttpsUrl(dapp.logo) ? (
            <img
              src={`chrome://image?${dapp.logo}`}
              width={72}
              height={72}
            />
          ) : (
            <PlaceholderImage size='72px' />
          )}

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
              metricName={getLocale('braveWalletActiveWallets')}
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
            {dapp.chains.map((chainId) => {
              const net = networks.find((n) => n.chainId === chainId)
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
        {isHttpsUrl(dapp.website) ? (
          <ExternalLink
            href={dapp.website}
            text={getLocale('braveWalletVisitDapp').replace('$1', dapp.name)}
            width='100%'
          />
        ) : null}
      </Column>
    </DappDetailDialog>
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
