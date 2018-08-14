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

const activityIcon = require('./assets/activity')

type Token = {
  tokens: number,
  converted: number
  isNegative?: boolean
}

export interface Props {
  onActivity: () => void
  grant?: Token
  ads?: Token
  contribute: Token
  donation?: Token
  tips?: Token
  total: Token
  id?: string
}

export default class WalletSummary extends React.PureComponent<Props, {}> {
  render () {
    const { id, grant, ads, contribute, donation, tips, onActivity, total } = this.props
    const date = new Date()
    const month = getLocale(`month${date.toLocaleString('en-us', { month: 'short' })}`)
    const year = date.getFullYear()

    return (
      <StyledWrapper id={id}>
        <StyledSummary>{getLocale('rewardsSummary')}</StyledSummary>
        <StyledTitle>{month} {year}</StyledTitle>
        <div>
          {
            grant
            ? <ListToken
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
              value={ads.tokens}
              converted={ads.converted}
              color={'earnings'}
              title={getLocale('earningsAds')}
            />
            : null
          }
          <ListToken
            value={contribute.tokens}
            converted={contribute.converted}
            color={'contribute'}
            title={getLocale('rewardsContribute')}
            isNegative={true}
          />
          {
            donation
            ? <ListToken
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
              value={tips.tokens}
              converted={tips.converted}
              color={'donation'}
              title={getLocale('oneTimeDonation')}
              isNegative={true}
            />
            : null
          }
          <ListToken
            value={total.tokens}
            converted={total.converted}
            border={'last'}
            title={getLocale('total')}
            isNegative={total.isNegative || false}
          />
        </div>
        <StyledActivity onClick={onActivity}>
          <StyledActivityIcon>{activityIcon}</StyledActivityIcon> {getLocale('viewMonthly')}
        </StyledActivity>
      </StyledWrapper>
    )
  }
}
