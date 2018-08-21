/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Helpers
import { getLocale } from '../../../helpers'

// Styled Components
import {
  StyledWrapper,
  StyledAttentionScore,
  StyledAttentionScoreTitle,
  StyledContainer,
  StyledScoreWrapper,
  StyledControlsWrapper,
  StyledDonateText,
  StyledIcon,
  StyledDonateWrapper,
  StyledToggleWrapper,
  StyledSelectWrapper
} from './style'

// Components
import {
  Grid,
  Column,
  Select,
  Toggle,
  ButtonSecondary
} from '../../../components'
import { Tokens } from '../'
import Profile, { Provider } from '../profile/index'
import ToggleTips from '../toggleTips/index'

// Assets
const alertIcon = require('./assets/alert')
const monthlyIcon = require('./assets/monthly')

export interface Props {
  id?: string
  platform?: Provider
  publisherImg?: string
  publisherName?: string
  isVerified?: boolean
  attentionScore?: string | null
  tipsEnabled: boolean
  includeInAuto: boolean
  monthlyAmount: number
  donationAmounts: number[]
  donationAction: () => void
  onToggleTips: () => void
  onAmountChange: () => void
  onIncludeInAuto: () => void
}

export default class WalletPanel extends React.PureComponent<Props, {}> {
  publisherInfo () {
    const publisherTitle = this.props.publisherName || ''

    return (
      <StyledWrapper>
        <Profile
          type={'big'}
          title={publisherTitle}
          provider={this.props.platform}
          src={this.props.publisherImg}
          verified={this.props.isVerified}
        />
      </StyledWrapper>
    )
  }

  getSelectTheme () {
    return {
      border: 'none',
      padding: '0px',
      background: 'inherit'
    }
  }

  donationDropDown () {
    const { donationAmounts } = this.props
    const monthlyAmount = this.props.monthlyAmount || 5

    if (!donationAmounts) {
      return null
    }

    return (
      <StyledSelectWrapper>
        <Select
          theme={this.getSelectTheme()}
          value={monthlyAmount.toString()}
          onChange={this.props.onAmountChange}
        >
          {donationAmounts.map((amount: number, index: number) => {
            return (
              <div
                key={`donationAmount-${index}`}
                data-value={amount.toString()}
              >
                <Tokens value={amount} size={'mini'} color={'donation'}/>
              </div>
            )
          })}
        </Select>
      </StyledSelectWrapper>
    )
  }

  donationControls () {
    return (
      <StyledWrapper>
        <Grid columns={6}>
          <Column size={5}>
            <StyledDonateText>
              {getLocale('donateMonthly')}
              <StyledIcon>
                {monthlyIcon}
              </StyledIcon>
            </StyledDonateText>
          </Column>
          <Column size={1}>
            {this.donationDropDown()}
          </Column>
        </Grid>
        <Grid columns={6}>
          <Column size={5}>
            <StyledDonateText>
              {getLocale('includeInAuto')}
              <StyledIcon>
                {alertIcon}
              </StyledIcon>
            </StyledDonateText>
          </Column>
          <Column size={1}>
            <StyledToggleWrapper>
              <Toggle
                size={'medium'}
                checked={this.props.includeInAuto}
                onToggle={this.props.onIncludeInAuto}
              />
            </StyledToggleWrapper>
          </Column>
        </Grid>
      </StyledWrapper>
    )
  }

  render () {
    const {
      id,
      platform,
      onToggleTips,
      attentionScore,
      tipsEnabled,
      donationAction
    } = this.props

    return (
      <StyledWrapper>
        <StyledContainer id={id}>
          {this.publisherInfo()}
          <StyledScoreWrapper>
            <Grid columns={6}>
              <Column size={5}>
                <StyledAttentionScoreTitle>
                  {getLocale('rewardsContributeAttention')}
                </StyledAttentionScoreTitle>
              </Column>
              <Column size={1}>
                <StyledAttentionScore>
                  {attentionScore}%
                </StyledAttentionScore>
              </Column>
            </Grid>
          </StyledScoreWrapper>
          <StyledControlsWrapper>
            {this.donationControls()}
          </StyledControlsWrapper>
          <StyledDonateWrapper>
            <ButtonSecondary
              size={'large'}
              color={'action'}
              onClick={donationAction}
              text={getLocale('donateNow')}
            />
          </StyledDonateWrapper>
        </StyledContainer>
        <ToggleTips
          id={'toggle-tips'}
          provider={platform}
          onToggleTips={onToggleTips}
          tipsEnabled={tipsEnabled}
        />
      </StyledWrapper>
    )
  }
}
