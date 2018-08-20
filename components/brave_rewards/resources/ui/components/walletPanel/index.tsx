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
  StyledVerified,
  StyledVerifiedIcon,
  StyledContainer,
  StyledScoreWrapper,
  StyledControlsWrapper,
  StyledDonateText,
  StyledVerifiedText,
  StyledIcon,
  StyledDonateWrapper,
  StyledToggleWrapper
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
const verifiedIcon = require('./assets/verified')

export interface Props {
  id?: string
  platform?: Provider
  publisherImg?: string
  publisherName?: string
  isVerified?: boolean
  attentionScore?: string | null
  donationAction: () => void
  onToggleTips: () => void
}

interface State {
  tipsEnabled: boolean
  includeInAuto: boolean
  monthlyAmount: number | null
}

export default class WalletPanel extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      tipsEnabled: true,
      includeInAuto: true,
      monthlyAmount: null
    }
  }

  onIncludeInAuto = () => {
    this.setState({
      includeInAuto: !this.state.includeInAuto
    })
  }

  verifiedSubTitle () {
    if (!this.props.isVerified) {
      return null
    }

    return (
      <StyledVerified>
        <StyledVerifiedIcon>
          {verifiedIcon}
        </StyledVerifiedIcon>
        <StyledVerifiedText>
          {getLocale('verifiedPublisher')}
        </StyledVerifiedText>
      </StyledVerified>
    )
  }

  publisherInfo () {
    return (
      <StyledWrapper>
        <Profile
          type={'big'}
          title={'Bart Baker'}
          provider={this.props.platform}
          src={this.props.publisherImg}
          subTitle={this.verifiedSubTitle()}
        />
      </StyledWrapper>
    )
  }

  getSelectTheme () {
    return {
      border: 'none',
      padding: '0px',
      margin: '3px 0px 0px 0px',
      background: 'inherit'
    }
  }

  donationDropDown () {
    return (
      <div>
        <Select theme={this.getSelectTheme()}>
          <div data-value='5'>
            <Tokens value={5} size={'mini'} color={'donation'}/>
          </div>
          <div data-value='10'>
            <Tokens value={10} size={'mini'} color={'donation'}/>
          </div>
          <div data-value='15'>
            <Tokens value={15} size={'mini'} color={'donation'}/>
          </div>
          <div data-value='20'>
            <Tokens value={20} size={'mini'} color={'donation'}/>
          </div>
          <div data-value='30'>
            <Tokens value={30} size={'mini'} color={'donation'}/>
          </div>
          <div data-value='50'>
            <Tokens value={50} size={'mini'} color={'donation'}/>
          </div>
          <div data-value='100'>
            <Tokens value={100} size={'mini'} color={'donation'}/>
          </div>
        </Select>
      </div>
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
                {monthlyIcon('#4C54D2')}
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
                highZ={true}
                size={'medium'}
                onToggle={this.onIncludeInAuto}
                checked={this.state.includeInAuto}
              />
            </StyledToggleWrapper>
          </Column>
        </Grid>
      </StyledWrapper>
    )
  }

  render () {
    const { id, platform, onToggleTips, attentionScore, donationAction } = this.props

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
              size={'full'}
              color={'donate'}
              onClick={donationAction}
              text={getLocale('donateNow')}
            />
          </StyledDonateWrapper>
        </StyledContainer>
        <ToggleTips
          id={'toggle-tips'}
          provider={platform}
          onToggleTips={onToggleTips}
          tipsEnabled={this.state.tipsEnabled}
        />
      </StyledWrapper>
    )
  }
}
