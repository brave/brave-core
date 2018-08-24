/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledSummary,
  StyledActivity,
  StyledActivityIcon
} from './style'
import ListToken from '../listToken/index'
import { getLocale } from '../../../helpers'
import { WalletActivityIcon } from '../../../components/icons'

type Token = {
  tokens: number,
  converted: number
  isNegative?: boolean
}

export interface Props {
  onActivity?: () => void
  grant?: Token
  deposit?: Token
  ads?: Token
  contribute: Token
  donation?: Token
  tips?: Token
  total: Token
  id?: string
  compact?: boolean
}

export default class WalletSummary extends React.PureComponent<Props, {}> {
  render () {
    const { id, grant, ads, contribute, donation, tips, onActivity, total, deposit, compact } = this.props
    const date = new Date()
    const month = getLocale(`month${date.toLocaleString('en-us', { month: 'short' })}`)
    const year = date.getFullYear()
    const tokenSize = compact ? 'small' : 'normal'

    return (
      <StyledWrapper id={id}>
        <StyledSummary>{getLocale('rewardsSummary')}</StyledSummary>
        <StyledTitle>{month} {year}</StyledTitle>
        <div>
          {
            grant
            ? <ListToken
              size={tokenSize}
              value={grant.tokens}
              converted={grant.converted}
              color={'earnings'}
              title={getLocale('tokenGrant')}
            />
            : null
          }
          {
            ads
            ? <ListToken
              size={tokenSize}
              value={ads.tokens}
              converted={ads.converted}
              color={'earnings'}
              title={getLocale('earningsAds')}
            />
            : null
          }
          {
            deposit
            ? <ListToken
              size={tokenSize}
              value={deposit.tokens}
              converted={deposit.converted}
              color={'earnings'}
              title={getLocale('deposits')}
            />
            : null
          }
          <ListToken
            size={tokenSize}
            value={contribute.tokens}
            converted={contribute.converted}
            color={'contribute'}
            title={getLocale('rewardsContribute')}
            isNegative={true}
          />
          {
            donation
            ? <ListToken
              size={tokenSize}
              value={donation.tokens}
              converted={donation.converted}
              color={'donation'}
              title={getLocale('recurringDonations')}
              isNegative={true}
            />
            : null
          }
          {
            tips
            ? <ListToken
              size={tokenSize}
              value={tips.tokens}
              converted={tips.converted}
              color={'donation'}
              title={getLocale('oneTimeDonation')}
              isNegative={true}
            />
            : null
          }
          <ListToken
            size={tokenSize}
            value={total.tokens}
            converted={total.converted}
            border={'last'}
            title={getLocale('total')}
            isNegative={total.isNegative || false}
          />
        </div>
        {
          onActivity
          ? <StyledActivity onClick={onActivity}>
            <StyledActivityIcon>
              <WalletActivityIcon />
            </StyledActivityIcon>
            {getLocale('viewMonthly')}
          </StyledActivity>
          : null
        }
      </StyledWrapper>
    )
  }
}
