/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Rewards Components
import Hero from '../hero'
import SettingsPage from '../settingsPage'
import ButtonCta from '../../../components/buttonsIndicators/buttonCta'
import ButtonSecondary from '../../../components/buttonsIndicators/buttonSecondary'
import { InfoCards, InfoCardProps } from '../infoCards'

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
  StyledFigure,
  StyledBatLogo,
  StyledDownArrow,
  StyledH1,
  StyledH2,
  StyledCenterTitle,
  StyledSubTitle,
  StyledTrademark,
  StyledRewardsParagraph,
  StyledTeaserParagraph,
  StyledReadyParagraph,
  StyledCenterParagraph,
  StyledBoldParagraph,
  StyledStrong,
  StyledAnchor
} from './style'

export interface WelcomePageProps {
  id?: string
  optInAction: () => void
  infoItems?: InfoCardProps[]
  pageImages?: WelcomePageImages
}

export interface WelcomePageImages {
  batLogo?: string,
  downArrow?: string
}

export interface BackgroundImageProps {
  src?: string
}

class WelcomePage extends React.PureComponent<WelcomePageProps, {}> {
  private centerTextSection: HTMLDivElement | null

  constructor (props: WelcomePageProps) {
    super(props)
    this.centerTextSection = null
    this.scrollToCenter = this.scrollToCenter.bind(this)
  }

  scrollToCenter () {
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

  get defaultImages (): WelcomePageImages {
    return {
      batLogo: '',
      downArrow: ''
    }
  }

  hero (pageImages: WelcomePageImages) {
    return (
      <Hero id={'rewards-hero'}>
        <StyledSection>
          <StyledFigure>
            <StyledBatLogo src={pageImages.batLogo}/>
          </StyledFigure>
          <StyledH1>
            {getLocale('braveRewardsTitle')}
          </StyledH1>
          <StyledTrademark>TM</StyledTrademark>
          <StyledSubTitle>
            {getLocale('braveRewardsSubTitle')}
          </StyledSubTitle>
          <StyledRewardsParagraph>
            {getLocale('braveRewardsDesc')}
          </StyledRewardsParagraph>
        </StyledSection>
        <StyledOptInSection>
          <ButtonSecondary
            size='large'
            color='subtle'
            text={getLocale('braveRewardsOptInText')}
            onClick={this.props.optInAction}
          />
        </StyledOptInSection>
        <StyledSection>
          <StyledTeaserParagraph>
            {getLocale('braveRewardsTeaser')}
          </StyledTeaserParagraph>
          <StyledAnchor onClick={this.scrollToCenter}>
            <StyledDownArrow src={pageImages.downArrow}/>
          </StyledAnchor>
        </StyledSection>
      </Hero>
    )
  }

  get centerTextContent () {
    return (
      <StyledCenterContent>
        <StyledSection>
          <StyledCenterTitle>
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
          <StyledCenterTitle>
            {getLocale('whyBraveRewards')}
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
        <StyledH2>
          {getLocale('readyToTakePart')}
        </StyledH2>
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
        <ButtonCta
          text={getLocale('readyToTakePartOptInText')}
          color={'brand'}
          onClick={this.props.optInAction}
        />
      </StyledOptInInnerSection>
    )
  }

  render () {
    const { id, infoItems, pageImages } = this.props
    const images = pageImages || this.defaultImages

    return (
      <SettingsPage id={id}>
        <StyledBackground>
          <StyledSection>
            {this.hero(images)}
          </StyledSection>
          <StyledCenterSection>
            <StyledCenterInner innerRef={this.refSet}>
              {this.centerTextContent}
            </StyledCenterInner>
            <StyledInfoContent>
              <InfoCards
                id='rewards-info'
                infoItems={infoItems}
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

export {
  WelcomePage
}
