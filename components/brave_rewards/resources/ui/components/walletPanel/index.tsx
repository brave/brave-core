/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Helpers
import { getLocale } from '../../../helpers'

// Styled Components
import {
  StyledWrapper,
  StyledSummary,
  StyledAttentionScore,
  StyledAttentionScoreTitle,
  StyledVerified,
  StyledVerifiedIcon,
  StyledContainer,
  StyledScoreWrapper,
  StyledControlsWrapper,
  StyledDonateText,
  StyledVerifiedText
} from './style'

// Components
import {
  Grid,
  Column,
  Select,
  Toggle,
  ButtonCta
} from '../../../components'
import { Tokens } from '../'
import Profile, { Provider } from '../profile/index'

// Assets
const verifiedIcon = require('./assets/verified')

export interface Props {
  id?: string
  platform?: Provider
  publisherImg?: string
  publisherName?: string
  isVerified?: boolean
  attentionScore?: string | null
  donationAction: () => void
}

interface State {
  includeInAuto: boolean
  tipsEnabled: boolean
  monthlyAmount: number | null
}

export default class WalletPanel extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      includeInAuto: true,
      tipsEnabled: true,
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

  donationDropDown () {
    return (
      <div>
        <Select theme={{ border: 'none', maxWidth: '50px' }}>
          <div data-value='5'>
            <Tokens value={5} color={'donation'}/>
          </div>
          <div data-value='10'>
            <Tokens value={10} color={'donation'}/>
          </div>
          <div data-value='15'>
            <Tokens value={15} color={'donation'}/>
          </div>
          <div data-value='20'>
            <Tokens value={20} color={'donation'}/>
          </div>
          <div data-value='30'>
            <Tokens value={30} color={'donation'}/>
          </div>
          <div data-value='50'>
            <Tokens value={50} color={'donation'}/>
          </div>
          <div data-value='100'>
            <Tokens value={100} color={'donation'}/>
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
            </StyledDonateText>
          </Column>
          <Column size={1}>
            <Toggle
              size={'medium'}
              onToggle={this.onIncludeInAuto}
              checked={this.state.includeInAuto}
            />
          </Column>
        </Grid>
      </StyledWrapper>
    )
  }

  render () {
    const { id, attentionScore, donationAction } = this.props

    return (
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
        <ButtonCta
          color={'action'}
          onClick={donationAction}
          text={getLocale('donateNow')}
        />
        <StyledSummary>
          {getLocale('rewardsSummary')}
        </StyledSummary>
      </StyledContainer>
    )
  }
}
