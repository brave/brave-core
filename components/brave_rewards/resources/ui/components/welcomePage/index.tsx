/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Hero from '../hero'
import SettingsPage from '../settingsPage'
import Button from '../../../components/buttonsIndicators/button'
import InfoCard, { CardProps } from '../infoCard'
import { ArrowDownIcon, BatColorIcon, LoaderIcon } from '../../../components/icons'
import Alert from '../alert'

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
  StyledCenterParagraph,
  StyledAnchor,
  StyledOptInSecond,
  StyledHeroInfo,
  StyledAlert,
  StyledAlertLeft,
  StyledAlertContent
} from './style'

const turnOnRewardsImage = require('./assets/turnOnRewards')
const braveAdsImage = require('./assets/braveAds')
const braveContributeImage = require('./assets/braveContribute')

export interface Props {
  id?: string
  optInAction: () => void
  creating?: boolean
  onReTry?: () => void
}

class WelcomePage extends React.PureComponent<Props, {}> {
  private isTouchScreen: boolean
  private centerTextSection: HTMLDivElement | null

  constructor (props: Props) {
    super(props)
    this.centerTextSection = null
    this.isTouchScreen = 'ontouchstart' in document.documentElement!
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

  optInAction = () => {
    this.props.optInAction()
  }

  hero () {
    return (
      <Hero
        id={'rewards-hero'}
        isMobile={this.isTouchScreen}
      >
        <StyledSection>
          <StyledBatLogo>
            <BatColorIcon />
          </StyledBatLogo>
          <StyledHeroInfo>
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
          </StyledHeroInfo>
        </StyledSection>
        <StyledOptInSection>
          {
            this.props.creating
            ? <Button
              level='secondary'
              size='call-to-action'
              type='subtle'
              text={getLocale('braveRewardsCreatingText')}
              disabled={true}
              data-test-id='optInAction'
              icon={{
                image: <LoaderIcon />,
                position: 'after'
              }}
            />
            : <Button
              level='secondary'
              size='call-to-action'
              type='subtle'
              text={getLocale('braveRewardsOptInText')}
              onClick={this.optInAction}
              data-test-id='optInAction'
            />
          }
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
        <StyledCenterInner>
          <StyledCenterTitle level={3}>
            {getLocale('whyBraveRewards')}
          </StyledCenterTitle>
          <StyledCenterParagraph>
            {getLocale('whyBraveRewardsDesc1')}
          </StyledCenterParagraph>
          <StyledCenterParagraph>
            {getLocale('whyBraveRewardsDesc2')}
          </StyledCenterParagraph>
        </StyledCenterInner>
      </StyledCenterContent>
    )
  }

  get optInContent () {
    return (
      <StyledOptInInnerSection>
        <StyledActionTitle level={4}>
          {getLocale('readyToTakePart')}
        </StyledActionTitle>
        <StyledOptInSecond>
          {
            this.props.creating
            ? <Button
              level={'primary'}
              size={'call-to-action'}
              type={'accent'}
              text={getLocale('braveRewardsCreatingText')}
              disabled={true}
              icon={{
                image: <LoaderIcon />,
                position: 'after'
              }}
            />
            : <Button
              level={'primary'}
              size={'call-to-action'}
              type={'accent'}
              text={getLocale('readyToTakePartOptInText')}
              onClick={this.optInAction}
            />
          }
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

  get welcomePageContent () {
    return (
      <>
        {
          this.props.onReTry
          ? <StyledAlert>
            <Alert type={'error'}>
              <StyledAlertContent>
                <StyledAlertLeft>
                  <b>{getLocale('walletFailedTitle')}</b><br/>{getLocale('walletFailedText')}
                </StyledAlertLeft>
                  <Button
                    level={'primary'}
                    type={'accent'}
                    text={getLocale('walletFailedButton')}
                    onClick={this.props.onReTry}
                  />
              </StyledAlertContent>
            </Alert>
          </StyledAlert>
          : null
        }
        <StyledBackground>
          <StyledSection>
            {this.hero()}
          </StyledSection>
          <StyledCenterSection>
            <StyledCenterSection innerRef={this.refSet}>
              {this.centerTextContent}
            </StyledCenterSection>
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
      </>
    )
  }

  render () {
    const { id } = this.props

    // We don't need the SettingsPage wrapper on touchscreen devices
    if (this.isTouchScreen) {
      return this.welcomePageContent
    }

    return (
      <SettingsPage id={id}>
        {this.welcomePageContent}
      </SettingsPage>
    )
  }
}

export default WelcomePage
