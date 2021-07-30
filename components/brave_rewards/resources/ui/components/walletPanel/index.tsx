/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Helpers
import { getLocale } from 'brave-ui/helpers'

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
  StyledGrid,
  StyledColumn,
  StyleToggleTips,
  StyledNoticeWrapper,
  StyledNoticeLink,
  StyledProfileWrapper,
  StyledMonthlyBorder,
  StyledMonthlyWrapper,
  StyledMonthlyAmount,
  StyledMonthlyActions,
  StyledMonthlyDownIcon,
  StyledSetButtonContainer,
  StyledSetButton
} from './style'

// Components
import { Toggle } from 'brave-ui/components'

import { RewardsButton } from '../'
import ToggleTips from '../toggleTips/index'
import Profile, { Provider } from '../profile/index'

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
  toggleTips?: boolean
  donationAction: () => void
  onToggleTips: () => void
  onIncludeInAuto: () => void
  onRefreshPublisher?: () => void
  refreshingPublisher?: boolean
  publisherRefreshed?: boolean
  showUnVerified?: boolean
  moreLink?: string
  acEnabled?: boolean
  setMonthlyAction: () => void
  cancelMonthlyAction: () => void
}

interface State {
  showMonthlyActions: boolean
}

export default class WalletPanel extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showMonthlyActions: false
    }
  }

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
          showUnVerified={true}
          refreshingPublisher={this.props.refreshingPublisher}
          onRefreshPublisher={this.props.onRefreshPublisher}
          publisherRefreshed={this.props.publisherRefreshed}
        />
      </StyledProfileWrapper>
    )
  }

  donationDropDown () {
    const { monthlyAmount } = this.props
    const { showMonthlyActions } = this.state
    const batFormatString = 'bat'

    if (!monthlyAmount) {
      return null
    }

    const toggleActionsShown = () => {
      this.setState({ showMonthlyActions: !this.state.showMonthlyActions })
    }

    const onBlur = (event: React.FocusEvent) => {
      // Hide the monthly contribution actions when focus has left the
      // containing HTML element.
      const { relatedTarget } = event
      if (relatedTarget && relatedTarget instanceof Node) {
        for (let e: Node | null = relatedTarget; e; e = e.parentNode) {
          if (e === event.currentTarget) {
            return
          }
        }
      }
      this.setState({ showMonthlyActions: false })
    }

    return (
      <StyledMonthlyWrapper>
        <StyledMonthlyBorder
          className={showMonthlyActions ? 'expanded' : ''}
          onBlur={onBlur}
        >
          <StyledMonthlyAmount>
            <button
              onClick={toggleActionsShown}
              data-test-id='toggle-monthly-actions'
            >
              {monthlyAmount} {getLocale(batFormatString)}
              {showMonthlyActions ? null : <StyledMonthlyDownIcon />}
            </button>
          </StyledMonthlyAmount>
          {
            showMonthlyActions &&
              <StyledMonthlyActions>
                <button
                  onClick={this.props.setMonthlyAction}
                  data-test-id='change-monthly-amount'
                >
                  {getLocale('changeAmount')}
                </button>
                <button
                  onClick={this.props.cancelMonthlyAction}
                  data-test-id='clear-monthly-amount'
                >
                  {getLocale('cancel')}
                </button>
              </StyledMonthlyActions>
          }
        </StyledMonthlyBorder>
      </StyledMonthlyWrapper>
    )
  }

  donationControls () {
    const { monthlyAmount, acEnabled, setMonthlyAction } = this.props

    const firstColSpan = monthlyAmount ? '5' : '4'
    const secColSpan = monthlyAmount ? '0' : '2'

    return (
      <StyledWrapper>
        {
          acEnabled
          ? <StyledGrid>
            <StyledColumn size={'5'} shrink={'1'}>
              <StyledDonateText>{getLocale('includeInAuto')}</StyledDonateText>
            </StyledColumn>
            <StyledColumn size={'0'}>
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
        <StyledGrid id={'panel-donate-monthly'}>
          <StyledColumn size={firstColSpan} shrink={'1'}>
            <StyledDonateText>
              {getLocale('donateMonthly')}
            </StyledDonateText>
          </StyledColumn>
          <StyledColumn size={secColSpan}>
            {
              monthlyAmount
              ? this.donationDropDown()
              : <StyledSetButtonContainer>
                  <StyledSetButton
                    data-test-id={'tip-monthly'}
                    onClick={setMonthlyAction}
                  >
                    {getLocale('set')}
                  </StyledSetButton>
                </StyledSetButtonContainer>
            }
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
            showUnVerified
            ? <StyledNoticeWrapper>
              {getLocale(isVerified ? 'connectedText' : 'unVerifiedText')}
              {' '}
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
                <StyledColumn size={'0'}>
                  <StyledAttentionScore data-test-id={'attention-score'}>
                    {attentionScore}%
                  </StyledAttentionScore>
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
              testId='tip-once'
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
