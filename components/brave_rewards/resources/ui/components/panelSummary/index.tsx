/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledSummary,
  StyledTokensWrapper,
  StyledGrantTitle,
  StyledGrantIcon,
  StyledGrant,
  StyledGrantText,
  StyledGrantClaim,
  StyledActivity,
  StyledActivityIcon,
  StyledGrantEmpty
} from './style'
import Tokens from '../tokens/index'
import ListToken from '../listToken/index'
import { getLocale } from '../../../helpers'

const coinsIcon = require('./assets/coins')
const activityIcon = require('./assets/activity')

type Token = {color: string, tokens: number, converted: number}
type Grant = {id: string, tokens: number, converted: number}

export interface Props {
  grant: Token
  ads: Token
  contribute: Token
  donation: Token
  tips: Token
  onActivity: () => void
  id?: string
  grants?: Grant[]
  onClaim?: (id: string) => void
}

export default class PanelSummary extends React.PureComponent<Props, {}> {
  render () {
    const { id, grant, ads, contribute, donation, tips, grants, onClaim, onActivity } = this.props
    const date = new Date()
    const month = getLocale(`month${date.toLocaleString('en-us', { month: 'short' })}`)
    const year = date.getFullYear()

    return (
      <StyledWrapper id={id}>
        <StyledSummary>{getLocale('rewardsSummary')}</StyledSummary>
        <StyledTitle>{month} {year}</StyledTitle>
        <StyledTokensWrapper>
          <ListToken
            value={grant.tokens}
            converted={grant.converted}
            theme={{ color: grant.color }}
            title={getLocale('tokenGrant')}
          />
          <ListToken
            value={ads.tokens}
            converted={ads.converted}
            theme={{ color: ads.color }}
            title={getLocale('earningsAds')}
          />
          <ListToken
            value={contribute.tokens}
            converted={contribute.converted}
            theme={{ color: contribute.color }}
            title={getLocale('rewardsContribute')}
            isNegative={true}
          />
          <ListToken
            value={donation.tokens}
            converted={donation.converted}
            theme={{ color: donation.color }}
            title={getLocale('recurringDonations')}
            isNegative={true}
          />
          <ListToken
            value={tips.tokens}
            converted={tips.converted}
            theme={{ color: tips.color, borderBottom: 'none' }}
            title={getLocale('oneTimeDonation')}
            isNegative={true}
          />
        </StyledTokensWrapper>
        <StyledGrantTitle>
          <StyledGrantIcon>{coinsIcon}</StyledGrantIcon> {getLocale('tokenGrant')}
        </StyledGrantTitle>
        {
          grants && grants.map((grant: Grant, i: number) => {
            return <StyledGrant key={`${id}-grant-${i}`}>
              <StyledGrantText>
                <Tokens
                  value={grant.tokens}
                  converted={grant.converted}
                  theme={{
                    color: {
                      text: 'rgba(255, 255, 255, 0.65)',
                      token: '#fff',
                      tokenNum: '#fff'
                    },
                    size: {
                      token: '22px',
                      text: '14px'
                    }
                  }}
                />
              </StyledGrantText>
              <StyledGrantClaim onClick={onClaim && onClaim.bind(this, grant.id)}>{getLocale('claim')}</StyledGrantClaim>
            </StyledGrant>
          })
        }
        {
          !grants || grants.length === 0
          ? <StyledGrantEmpty>{getLocale('noGrants')}</StyledGrantEmpty>
          : null
        }
        <StyledActivity onClick={onActivity}>
          <StyledActivityIcon>{activityIcon}</StyledActivityIcon> {getLocale('walletActivity')}
        </StyledActivity>
      </StyledWrapper>
    )
  }
}
