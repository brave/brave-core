/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import {
  StyledWrapper,
  StyledDonation,
  StyledContent,
  StyledBanner,
  StyledBannerFiller,
  StyledBannerImage,
  StyledClose,
  StyledContentWrapper,
  StyledLogoWrapper,
  StyledLogoBorder,
  StyledTextWrapper,
  StyledTitle,
  StyledText,
  StyledWallet,
  StyledTokens,
  StyledCenter,
  StyledUserName,
  StyledScreenName,
  StyledSocialItem,
  StyledSocialIcon,
  StyledOption,
  StyledLogoText,
  StyledSocialWrapper,
  StyledEmptyBox,
  StyledLogoImage,
  StyledCheckbox,
  StyledNoticeWrapper,
  StyledNoticeIcon,
  StyledNoticeText,
  StyledNoticeLink,
  StyledVerifiedIcon
} from './style'

import Donate from '../donate/index'
import Checkbox from 'brave-ui/components/formControls/checkbox/index'
import { getLocale } from 'brave-ui/helpers'
import {
  CloseCircleOIcon,
  TwitterColorIcon,
  YoutubeColorIcon,
  TwitchColorIcon,
  RedditColorIcon,
  GitHubColorIcon,
  AlertCircleIcon,
  VerifiedSIcon
} from 'brave-ui/components/icons'

export type Social = { type: SocialType, url: string }
export type SocialType = 'twitter' | 'youtube' | 'twitch' | 'reddit' | 'vimeo' | 'github'
export type Donation = { tokens: string, converted: string, selected?: boolean }

export interface Props {
  balance: string
  currentAmount: string
  donationAmounts: Donation[]
  onAmountSelection: (tokens: string) => void
  id?: string
  title?: string
  name?: string
  screenName?: string
  domain: string
  bgImage?: string
  logo?: string
  social?: Social[]
  provider?: SocialType
  recurringDonation?: boolean
  children?: React.ReactNode
  onDonate: (amount: string, monthly: boolean) => void
  onClose?: () => void
  logoBgColor?: CSS.Color
  showUnVerifiedNotice?: boolean
  learnMoreNotice?: string
  addFundsLink?: string
  isVerified?: boolean
  onlyAnonWallet?: boolean
}

interface State {
  monthly: boolean
}

export default class SiteBanner extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      monthly: false
    }
  }

  getLogo (logo: string | undefined, domain: string, name: string | undefined) {
    let letter = domain && domain.substring(0, 1) || ''

    if (name) {
      letter = name.substring(0, 1)
    }

    return !logo
      ? <StyledLogoText>{letter}</StyledLogoText>
      : <StyledLogoImage bg={logo} />
  }

  getSocialData (item: Social) {
    let logo = null
    switch (item.type) {
      case 'twitter':
        logo = <TwitterColorIcon />
        break
      case 'youtube':
        logo = <YoutubeColorIcon />
        break
      case 'twitch':
        logo = <TwitchColorIcon />
        break
      case 'github':
        logo = <GitHubColorIcon />
        break
      case 'reddit':
        logo = <RedditColorIcon />
        break
    }

    return logo
  }

  getSocial = (social?: Social[]) => {
    if (!social || social.length === 0) {
      return null
    }

    const self = this
    return social.map((item: Social) => {
      const logo = self.getSocialData(item)
      return (
        <StyledSocialItem
          key={`${self.props.id}-social-${item.type}`}
          href={item.url}
          target={'_blank'}
        >
          <StyledSocialIcon>
            {logo}
          </StyledSocialIcon>
        </StyledSocialItem>
      )
    })
  }

  getTitle (title?: string) {
    return title ? title : getLocale('welcome')
  }

  getBannerTitle (name?: string, domain?: string, provider?: SocialType) {
    const identifier = name || domain

    if (!provider) {
      return identifier
    }

    switch (provider) {
      case 'youtube':
        return `${identifier} ${getLocale('on')} YouTube`
      case 'twitter':
        return `${identifier} ${getLocale('on')} Twitter`
      case 'twitch':
        return `${identifier} ${getLocale('on')} Twitch`
      case 'reddit':
        return `${identifier} ${getLocale('on')} Reddit`
      case 'vimeo':
        return `${identifier} ${getLocale('on')} Vimeo`
      case 'github':
        return `${identifier} ${getLocale('on')} GitHub`
      default:
        return identifier
    }
  }

  getBannerImageContent (name?: string, screenName?: string, domain?: string, provider?: SocialType) {
    if (screenName) {
      return (
        <>
          <StyledUserName>
            {this.getBannerTitle(name, domain, provider)}
          </StyledUserName>
          <StyledScreenName>
            {screenName}
          </StyledScreenName>
        </>
      )
    } else {
      return (
        <StyledCenter>
          {this.getBannerTitle(name, domain, provider)}
        </StyledCenter>
      )
    }
  }

  getText (children?: React.ReactNode) {
    if (!children) {
      return (
        <>
          <p>
            {getLocale('rewardsBannerText1')}
          </p>
          <p>
            {getLocale('rewardsBannerText2')}
          </p>
        </>
      )
    }

    return children
  }

  onMonthlyChange = (key: string, selected: boolean) => {
    this.setState({ monthly: selected })
  }

  onDonate = (amount: string) => {
    if (this.props.onDonate) {
      this.props.onDonate(amount, this.state.monthly)
    }
  }

  onKeyUp = (e: React.KeyboardEvent<HTMLDivElement>) => {
    if (e.key.toLowerCase() === 'escape' && this.props.onClose) {
      this.props.onClose()
    }
  }

  render () {
    const {
      id,
      bgImage,
      onClose,
      logo,
      social,
      provider,
      children,
      title,
      recurringDonation,
      balance,
      donationAmounts,
      domain,
      onAmountSelection,
      logoBgColor,
      currentAmount,
      name,
      screenName,
      showUnVerifiedNotice,
      learnMoreNotice,
      addFundsLink,
      isVerified,
      onlyAnonWallet
    } = this.props

    const isTwitterTip: boolean = !!(screenName && screenName !== '')
    const batFormatString = onlyAnonWallet ? 'bap' : 'bat'

    return (
      <StyledWrapper
        id={id}
        onKeyUp={this.onKeyUp}
        tabIndex={0}
      >
        <StyledBanner>
          <StyledClose onClick={onClose}>
            <CloseCircleOIcon />
          </StyledClose>
          <StyledBannerImage bgImage={bgImage}>
            {
              !bgImage
              ? this.getBannerImageContent(name, screenName, domain, provider)
              : <StyledBannerFiller />
            }
          </StyledBannerImage>
          <StyledContentWrapper>
            <StyledContent>
              <StyledLogoWrapper>
                {
                  isVerified
                  ? <StyledVerifiedIcon>
                    <VerifiedSIcon/>
                  </StyledVerifiedIcon>
                  : null
                }

                <StyledLogoBorder
                  padding={!logo}
                  bg={logoBgColor}
                >
                  {this.getLogo(logo, domain, name)}
                </StyledLogoBorder>
              </StyledLogoWrapper>
              <StyledTextWrapper>
                <StyledSocialWrapper>
                  {this.getSocial(social)}
                </StyledSocialWrapper>
                {
                  showUnVerifiedNotice
                    ? <StyledNoticeWrapper>
                      <StyledNoticeIcon>
                        <AlertCircleIcon />
                      </StyledNoticeIcon>
                      <StyledNoticeText>
                        <b>{getLocale('siteBannerNoticeNote')}</b>{' '}
                        {getLocale(isVerified ? 'siteBannerConnectedText' : 'siteBannerNoticeText')}
                        <StyledNoticeLink href={learnMoreNotice} target={'_blank'}>{getLocale('unVerifiedTextMore')}</StyledNoticeLink>
                      </StyledNoticeText>
                    </StyledNoticeWrapper>
                    : null
                }
                <StyledTitle isTwitterTip={isTwitterTip}>
                  {this.getTitle(title)}
                </StyledTitle>
                <StyledText>
                  {this.getText(children)}
                </StyledText>
              </StyledTextWrapper>
            </StyledContent>
            <StyledDonation>
              <StyledWallet>
                {getLocale('walletBalance')} <StyledTokens>{balance} {getLocale(batFormatString)}</StyledTokens>
              </StyledWallet>
              <Donate
                balance={parseFloat(balance)}
                donationAmounts={donationAmounts}
                title={getLocale('donationAmount')}
                onDonate={this.onDonate}
                actionText={this.state.monthly ? getLocale('doMonthly') : getLocale('sendDonation')}
                onAmountSelection={onAmountSelection}
                donateType={'big'}
                currentAmount={currentAmount}
                addFundsLink={addFundsLink}
                onlyAnonWallet={onlyAnonWallet}
              >
                {
                  !recurringDonation
                    ? <StyledCheckbox>
                      <Checkbox
                        testId={'monthlyCheckbox'}
                        value={{ make: this.state.monthly }}
                        onChange={this.onMonthlyChange}
                        type={'dark'}
                      >
                        <div data-key='make'>
                          <StyledOption>{getLocale('makeMonthly')}</StyledOption>
                        </div>
                      </Checkbox>
                    </StyledCheckbox>
                    : <StyledEmptyBox />
                }
              </Donate>
            </StyledDonation>
          </StyledContentWrapper>
        </StyledBanner>
      </StyledWrapper>
    )
  }
}
