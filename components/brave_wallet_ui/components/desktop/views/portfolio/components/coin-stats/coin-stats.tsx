// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// React
import { getLocale } from '$web-common/locale'

// Utils
import Amount from '../../../../../../utils/amount'

// Styled components
import {
  Currency,
  Row,
  StatLabel,
  StatValue,
  StatWrapper,
  StyledWrapper
} from './coin-stats-styles'
import { DividerText, SubDivider } from '../../style'

interface Props {
  marketCapRank: number
  volume: number
  marketCap: number
}

export const CoinStats = (props: Props) => {
  const { marketCapRank, marketCap, volume } = props
  const formattedMarketCap = new Amount(marketCap).abbreviate(2, undefined, 'billion')
  const formattedVolume = new Amount(volume).abbreviate(2, undefined, 'billion')

  return (
    <StyledWrapper>
      <DividerText>{getLocale('braveWalletInformation')}</DividerText>
      <SubDivider />
      <Row>
        <StatWrapper>
          <StatValue>{marketCapRank}</StatValue>
          <StatLabel>{getLocale('braveWalletRankStat')}</StatLabel>
        </StatWrapper>

        <StatWrapper>
          <StatValue>
            <Currency>$</Currency>
            {formattedVolume}
          </StatValue>
          <StatLabel>{getLocale('braveWalletVolumeStat')}</StatLabel>
        </StatWrapper>

        <StatWrapper>
          <StatValue>
            <Currency>$</Currency>
            {formattedMarketCap}
          </StatValue>
          <StatLabel>{getLocale('braveWalletMarketCapStat')}</StatLabel>
        </StatWrapper>
      </Row>
    </StyledWrapper>
  )
}
