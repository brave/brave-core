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
  StyledSelectWrapper,
  StyledGrid,
  StyledColumn
} from './style'

// Components
import {
  Select,
  Toggle,
  Button
} from '../../../components'

import { Tokens } from '../'
import ToggleTips from '../toggleTips/index'
import Profile, { Provider } from '../profile/index'
import { AlertCircleIcon, RefreshIcon } from '../../../components/icons'

export type Token = {
  tokens: string,
  converted: string
}

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
  donationAmounts: Token[]
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

  donationDropDown () {
    const { donationAmounts } = this.props
    const monthlyAmount = this.props.monthlyAmount || 5

    if (!donationAmounts) {
      return null
    }

    return (
      <StyledSelectWrapper>
        <Select
          floating={true}
          showAllContents={true}
          value={monthlyAmount.toString()}
          onChange={this.props.onAmountChange}
        >
          {donationAmounts.map((token: Token, index: number) => {
            return (
              <div
                key={`donationAmount-${index}`}
                data-value={token.tokens}
              >
                <Tokens
                  size={'mini'}
                  value={token.tokens}
                  converted={token.converted}
                  color={'donation'}
                />
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
        <StyledGrid>
          <StyledColumn size={'5'}>
            <StyledDonateText>
              {getLocale('donateMonthly')}
            </StyledDonateText>
            <StyledIcon>
              <RefreshIcon color={'#4C54D2'}/>
            </StyledIcon>
          </StyledColumn>
          <StyledColumn size={'1'}>
            {this.donationDropDown()}
          </StyledColumn>
        </StyledGrid>
        <StyledGrid>
          <StyledColumn size={'5'}>
            <StyledDonateText>
              {getLocale('includeInAuto')}
            </StyledDonateText>
            <StyledIcon>
              <AlertCircleIcon/>
            </StyledIcon>
          </StyledColumn>
          <StyledColumn size={'1'}>
            <StyledToggleWrapper>
              <Toggle
                size={'small'}
                checked={this.props.includeInAuto}
                onToggle={this.props.onIncludeInAuto}
              />
            </StyledToggleWrapper>
          </StyledColumn>
        </StyledGrid>
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
            <StyledGrid>
              <StyledColumn size={'5'}>
                <StyledAttentionScoreTitle>
                  {getLocale('rewardsContributeAttentionScore')}
                </StyledAttentionScoreTitle>
              </StyledColumn>
              <StyledColumn size={'1'}>
                <StyledAttentionScore>
                  {attentionScore}%
                </StyledAttentionScore>
              </StyledColumn>
            </StyledGrid>
          </StyledScoreWrapper>
          <StyledControlsWrapper>
            {this.donationControls()}
          </StyledControlsWrapper>
          <StyledDonateWrapper>
            <Button
              size={'call-to-action'}
              type={'accent'}
              level={'secondary'}
              brand={'rewards'}
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
