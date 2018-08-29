/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Hero from '../hero'
import SettingsPage from '../settingsPage'
import Button from '../../../components/buttonsIndicators/button'
import InfoCard, { CardProps } from '../infoCard'

// Utils
import { getLocale } from '../../../helpers'

// Styles
import {
  StyledOptInSection,
  StyledOptInInnerSection,
  StyledCenterSection,
  StyledCenterContent,
  StyledSection,
  StyledCenterInner,
  StyledInfoContent,
  StyledTakeActionContent,
  StyledBackground,
  StyledBatLogo,
  StyledRewardsTitle,
  StyledActionTitle,
  StyledCenterTitle,
  StyledSubTitle,
  StyledTrademark,
  StyledRewardsParagraph,
  StyledTeaserParagraph,
  StyledReadyParagraph,
  StyledCenterParagraph,
  StyledBoldParagraph,
  StyledStrong,
  StyledAnchor,
  StyledOptInSecond
} from './style'
import { ArrowDownIcon, BatColorIcon } from '../../../components/icons'

const turnOnRewardsImage = require('./assets/turnOnRewards')
const braveAdsImage = require('./assets/braveAds')
const braveContributeImage = require('./assets/braveContribute')

export interface Props {
  id?: string
  optInAction: () => void
}

class WelcomePage extends React.PureComponent<Props, {}> {
  private centerTextSection: HTMLDivElement | null

  constructor (props: Props) {
    super(props)
    this.centerTextSection = null
  }

  scrollToCenter = () => {
    if (!this.centerTextSection) {
      return
    }

    const centerTextSection = this.centerTextSection
    if (centerTextSection) {
      window.scrollTo({
        behavior: 'smooth',
        top: centerTextSection.offsetTop
      })
    }
  }

  refSet = (node: HTMLDivElement) => {
    this.centerTextSection = node
  }

  hero () {
    return (
      <Hero id={'rewards-hero'}>
        <StyledSection>
          <StyledBatLogo>
            <BatColorIcon />
          </StyledBatLogo>
          <StyledRewardsTitle level={2}>
            {getLocale('braveRewardsTitle')}
          </StyledRewardsTitle>
          <StyledTrademark>TM</StyledTrademark>
          <StyledSubTitle level={4}>
            {getLocale('braveRewardsSubTitle')}
          </StyledSubTitle>
          <StyledRewardsParagraph>
            {getLocale('braveRewardsDesc')}
          </StyledRewardsParagraph>
        </StyledSection>
        <StyledOptInSection>
          <Button
            level='secondary'
            size='call-to-action'
            type='subtle'
            text={getLocale('braveRewardsOptInText')}
            onClick={this.props.optInAction}
            data-test-id='optInAction'
          />
        </StyledOptInSection>
        <StyledSection>
          <StyledTeaserParagraph>
            {getLocale('braveRewardsTeaser')}
          </StyledTeaserParagraph>
          <StyledAnchor onClick={this.scrollToCenter}>
            <ArrowDownIcon />
          </StyledAnchor>
        </StyledSection>
      </Hero>
    )
  }

  get centerTextContent () {
    return (
      <StyledCenterContent>
        <StyledSection>
          <StyledCenterTitle level={3}>
            {getLocale('whyBraveRewards')}
          </StyledCenterTitle>
          <StyledCenterParagraph>
            {getLocale('whyBraveRewardsDesc')}
          </StyledCenterParagraph>
          <StyledBoldParagraph>
            {getLocale('whyBraveRewardsBold')}
          </StyledBoldParagraph>
        </StyledSection>
        <StyledSection>
          <StyledCenterTitle level={3}>
            {getLocale('howDoesItWork')}
          </StyledCenterTitle>
          <StyledCenterParagraph>
            {getLocale('howDoesItWorkDesc')}
          </StyledCenterParagraph>
        </StyledSection>
      </StyledCenterContent>
    )
  }

  get optInContent () {
    return (
      <StyledOptInInnerSection>
        <StyledActionTitle level={4}>
          {getLocale('readyToTakePart')}
        </StyledActionTitle>
        <StyledReadyParagraph>
          <span>
            {getLocale('readyToTakePartStart')}
          </span>
          <StyledStrong>
            {getLocale('readyToTakePartBold')}
          </StyledStrong>
          <span>
            {getLocale('readyToTakePartDesc')}
          </span>
        </StyledReadyParagraph>
        <StyledOptInSecond>
          <Button
            level={'primary'}
            size={'call-to-action'}
            type={'accent'}
            text={getLocale('readyToTakePartOptInText')}
            onClick={this.props.optInAction}
          />
        </StyledOptInSecond>
      </StyledOptInInnerSection>
    )
  }

  get infoCards (): CardProps[] {
    return [
      {
        title: getLocale('turnOnRewardsTitle'),
        description: getLocale('turnOnRewardsDesc'),
        icon: turnOnRewardsImage
      },
      {
        title: getLocale('braveAdsTitle'),
        description: getLocale('braveAdsDesc'),
        icon: braveAdsImage
      },
      {
        title: getLocale('braveContributeTitle'),
        description: getLocale('braveContributeDesc'),
        icon: braveContributeImage
      }
    ]
  }

  render () {
    const { id } = this.props

    return (
      <SettingsPage id={id}>
        <StyledBackground>
          <StyledSection>
            {this.hero()}
          </StyledSection>
          <StyledCenterSection>
            <StyledCenterInner innerRef={this.refSet}>
              {this.centerTextContent}
            </StyledCenterInner>
            <StyledInfoContent>
              <InfoCard
                id='rewards-info'
                cards={this.infoCards}
              />
            </StyledInfoContent>
            <StyledTakeActionContent>
              {this.optInContent}
            </StyledTakeActionContent>
          </StyledCenterSection>
        </StyledBackground>
      </SettingsPage>
    )
  }
}

export default WelcomePage
