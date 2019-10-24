/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledInner,
  StyledTitle,
  StyledSummary,
  StyledActivity,
  StyledActivityIcon,
  StyledNoActivity,
  StyledNoActivityWrapper,
  StyledReservedWrapper,
  StyledReservedLink,
  StyledAllReserved,
  StyledBatPoints
} from './style'
import ListToken from '../listToken'
import { Type } from '../tokens'
import { getLocale } from 'brave-ui/helpers'
import { WalletInfoIcon } from 'brave-ui/components/icons'

type Token = {
  tokens: string
  converted: string
  isNegative?: boolean
}

export interface Props {
  onActivity?: () => void
  report: {
    grant?: Token
    deposit?: Token
    ads?: Token
    contribute?: Token
    donation?: Token
    tips?: Token
  }
  id?: string
  compact?: boolean
  reservedAmount?: number
  reservedMoreLink?: string
  onlyAnonWallet?: boolean
  onSeeAllReserved?: () => void
}

export default class WalletSummary extends React.PureComponent<Props, {}> {
  generateList = () => {
    const { onlyAnonWallet } = this.props
    const tokenSize = this.props.compact ? 'small' : 'normal'
    const list = [
      {
        key: 'grant',
        translation: 'tokenGrantClaimed',
        color: 'earnings'
      },
      {
        key: 'ads',
        translation: 'earningsAds',
        color: 'earnings'
      },
      {
        key: 'deposit',
        translation: 'deposits',
        color: 'earnings'
      },
      {
        key: 'contribute',
        translation: 'rewardsContribute',
        color: 'contribute',
        negative: true
      },
      {
        key: 'donation',
        translation: 'recurringDonations',
        color: 'donation',
        negative: true
      },
      {
        key: 'tips',
        translation: 'oneTimeDonation',
        color: 'donation',
        negative: true
      }
    ]

    let result: React.ReactNode[] = []
    const all = Object.keys(this.props.report).length
    let current = 0

    list.forEach((item, index) => {
      const data = (this.props.report as Record<string, Token>)[item.key]
      if (data) {
        current++
        result.push((
          <ListToken
            testId={`summary-${item.key}`}
            key={`summary-${tokenSize}-${index}-${data.tokens}`}
            size={tokenSize}
            value={data.tokens}
            converted={data.converted}
            color={item.color as Type}
            title={getLocale(item.translation)}
            isNegative={item.negative}
            onlyAnonWallet={onlyAnonWallet}
            border={all === current ? 'last' : undefined}
          />
        ))
      }
    })

    if (result.length === 0) {
      return (
        <StyledNoActivityWrapper>
          <StyledNoActivity>
            {getLocale('noActivity')}
          </StyledNoActivity>
        </StyledNoActivityWrapper>
      )
    }

    return result
  }

  generateInfo = () => {
    const {
      reservedAmount,
      onlyAnonWallet,
      onSeeAllReserved,
      reservedMoreLink
    } = this.props
    const showReserved = reservedAmount && reservedAmount > 0

    if (!onlyAnonWallet && !showReserved) {
      return null
    }

    const amount = (reservedAmount && reservedAmount.toFixed(1)) || '0.0'
    const batFormatString = onlyAnonWallet ? getLocale('batPoints') : getLocale('bat')

    return (
      <StyledReservedWrapper data-test-id={'pending-contribution-box'}>
        {
          onlyAnonWallet
          ? <p>
              <StyledBatPoints>{getLocale('batPoints')}</StyledBatPoints> {getLocale('batPointsMessage')}
            </p>
          : null
        }
        {
          showReserved
          ? <>
              {getLocale('reservedAmountText', { reservedAmount: amount, currency: batFormatString })} <StyledReservedLink href={reservedMoreLink} target={'_blank'}>
                {getLocale('reservedMoreLink')}
              </StyledReservedLink>
              {
                onSeeAllReserved
                ? <StyledAllReserved onClick={onSeeAllReserved} data-test-id={'reservedAllLink'}>
                  {getLocale('reservedAllLink')}
                </StyledAllReserved>
                : null
              }
            </>
          : null
        }
      </StyledReservedWrapper>
    )
  }

  render () {
    const {
      id,
      onActivity,
      compact
    } = this.props
    const date = new Date()
    const month = getLocale(`month${date.toLocaleString('en-us', { month: 'short' })}`)
    const year = date.getFullYear()

    return (
      <StyledWrapper
        id={id}
        compact={compact}
      >
        <StyledInner>
          <StyledSummary>{getLocale('rewardsSummary')}</StyledSummary>
          <StyledTitle>{month} {year}</StyledTitle>
          <div>
            {this.generateList()}
            {this.generateInfo()}
          </div>
          {
            onActivity
            ? <StyledActivity onClick={onActivity}>
              <StyledActivityIcon>
                <WalletInfoIcon />
              </StyledActivityIcon>
              {getLocale('viewMonthly')}
            </StyledActivity>
            : null
          }
        </StyledInner>
      </StyledWrapper>
    )
  }
}
