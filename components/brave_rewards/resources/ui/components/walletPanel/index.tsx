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
  StyledDonateWrapper,
  StyledToggleWrapper,
  StyledSelectWrapper,
  StyledGrid,
  StyledColumn,
  StyleToggleTips,
  StyledNoticeWrapper,
  StyledNoticeLink,
  StyledProfileWrapper
} from './style'

// Components
import { Select, Toggle } from '../../../components'

import { RewardsButton, Tokens } from '../'
import ToggleTips from '../toggleTips/index'
import Profile, { Provider } from '../profile/index'

export type Token = {
  tokens: string
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
  monthlyAmount: string
  donationAmounts?: Token[]
  toggleTips?: boolean
  donationAction: () => void
  onToggleTips: () => void
  onAmountChange: () => void
  onIncludeInAuto: () => void
  onRefreshPublisher?: () => void
  refreshingPublisher?: boolean
  publisherRefreshed?: boolean
  showUnVerified?: boolean
  moreLink?: string
  acEnabled?: boolean
}

export default class WalletPanel extends React.PureComponent<Props, {}> {
  publisherInfo () {
    const publisherTitle = this.props.publisherName || ''

    return (
      <StyledProfileWrapper>
        <Profile
          type={'big'}
          title={publisherTitle}
          provider={this.props.platform}
          src={this.props.publisherImg}
          verified={this.props.isVerified}
          showUnVerifiedHelpIcon={
            !this.props.isVerified && this.props.showUnVerified
          }
          refreshingPublisher={this.props.refreshingPublisher}
          onRefreshPublisher={this.props.onRefreshPublisher}
          publisherRefreshed={this.props.publisherRefreshed}
        />
      </StyledProfileWrapper>
    )
  }

  donationDropDown () {
    const { donationAmounts } = this.props
    const monthlyAmount = this.props.monthlyAmount || '5.0'

    if (!donationAmounts) {
      return null
    }

    return (
      <StyledSelectWrapper>
        <Select
          floating={true}
          showAllContents={true}
          value={monthlyAmount}
          onChange={this.props.onAmountChange}
        >
          {donationAmounts.map((token: Token, index: number) => {
            return (
              <div key={`donationAmount-${index}`} data-value={token.tokens}>
                <Tokens
                  size={'small'}
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
    const { donationAmounts, acEnabled } = this.props

    if (!donationAmounts && !acEnabled) {
      return null
    }

    return (
      <StyledWrapper>
        {
          acEnabled
          ? <StyledGrid>
            <StyledColumn size={'5'}>
              <StyledDonateText>{getLocale('includeInAuto')}</StyledDonateText>
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
          : null
        }
        {
          donationAmounts
          ? <StyledGrid>
            <StyledColumn size={'5'}>
              <StyledDonateText>{getLocale('donateMonthly')}</StyledDonateText>
            </StyledColumn>
            <StyledColumn size={'1'}>{this.donationDropDown()}</StyledColumn>
          </StyledGrid>
          : null
        }
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
      donationAction,
      toggleTips,
      showUnVerified,
      isVerified,
      moreLink,
      acEnabled
    } = this.props

    const donationControls = this.donationControls()

    return (
      <StyledWrapper>
        <StyledContainer id={id}>
          {this.publisherInfo()}
          {
            !isVerified && showUnVerified
            ? <StyledNoticeWrapper>
              {getLocale('unVerifiedText')}{' '}
              <StyledNoticeLink href={moreLink} target={'_blank'}>
                {' '}
                {getLocale('unVerifiedTextMore')}
              </StyledNoticeLink>
            </StyledNoticeWrapper>
            : null
          }
          {
            acEnabled
            ? <StyledScoreWrapper>
              <StyledGrid>
                <StyledColumn size={'5'}>
                  <StyledAttentionScoreTitle>
                    {getLocale('rewardsContributeAttentionScore')}
                  </StyledAttentionScoreTitle>
                </StyledColumn>
                <StyledColumn size={'1'}>
                  <StyledAttentionScore>{attentionScore}%</StyledAttentionScore>
                </StyledColumn>
              </StyledGrid>
            </StyledScoreWrapper>
            : null
          }
          {
            donationControls
            ? <StyledControlsWrapper>
              {donationControls}
            </StyledControlsWrapper>
            : null
          }
          <StyledDonateWrapper>
            <RewardsButton
              type={'tip'}
              onClick={donationAction}
              text={getLocale('donateNow')}
            />
          </StyledDonateWrapper>
        </StyledContainer>
        <StyleToggleTips toggleTips={toggleTips}>
          <ToggleTips
            id={'toggle-tips'}
            provider={platform}
            onToggleTips={onToggleTips}
            tipsEnabled={tipsEnabled}
          />
        </StyleToggleTips>
      </StyledWrapper>
    )
  }
}
