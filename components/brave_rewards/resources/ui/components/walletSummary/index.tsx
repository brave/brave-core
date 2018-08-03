/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledSummary,
  StyledTokensWrapper,
  StyledActivity,
  StyledActivityIcon
} from './style'
import ListToken from '../listToken/index'
import { getLocale } from '../../../helpers'

const activityIcon = require('./assets/activity')

type Token = {color: string, tokens: number, converted: number}

export interface Props {
  onActivity: () => void
  grant?: Token
  ads?: Token
  contribute: Token
  donation?: Token
  tips?: Token
  id?: string
}

export default class WalletSummary extends React.PureComponent<Props, {}> {
  render () {
    const { id, grant, ads, contribute, donation, tips, onActivity } = this.props
    const date = new Date()
    const month = getLocale(`month${date.toLocaleString('en-us', { month: 'short' })}`)
    const year = date.getFullYear()

    return (
      <StyledWrapper id={id}>
        <StyledSummary>{getLocale('rewardsSummary')}</StyledSummary>
        <StyledTitle>{month} {year}</StyledTitle>
        <StyledTokensWrapper>
          {
            grant
            ? <ListToken
              value={grant.tokens}
              converted={grant.converted}
              theme={{ color: grant.color }}
              title={getLocale('tokenGrant')}
            />
            : null
          }
          {
            ads
            ? <ListToken
              value={ads.tokens}
              converted={ads.converted}
              theme={{ color: ads.color }}
              title={getLocale('earningsAds')}
            />
            : null
          }
          <ListToken
            value={contribute.tokens}
            converted={contribute.converted}
            theme={{ color: contribute.color }}
            title={getLocale('rewardsContribute')}
            isNegative={true}
          />
          {
            donation
            ? <ListToken
              value={donation.tokens}
              converted={donation.converted}
              theme={{ color: donation.color }}
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
              theme={{ color: tips.color, borderBottom: 'none' }}
              title={getLocale('oneTimeDonation')}
              isNegative={true}
            />
            : null
          }
        </StyledTokensWrapper>
        <StyledActivity onClick={onActivity}>
          <StyledActivityIcon>{activityIcon}</StyledActivityIcon> {getLocale('walletActivity')}
        </StyledActivity>
      </StyledWrapper>
    )
  }
}
