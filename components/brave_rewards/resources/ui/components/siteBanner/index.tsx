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
  StyledSocialItem,
  StyledSocialIcon,
  StyledOption,
  StyledLogoText,
  StyledSocialWrapper,
  StyledEmptyBox,
  StyledLogoImage,
  StyledCheckbox
} from './style'

import Donate from '../donate/index'
import Checkbox from '../../../components/formControls/checkbox/index'
import { getLocale } from '../../../helpers'
import {
  CloseStrokeIcon,
  TwitterColorIcon,
  YoutubeColorIcon,
  TwitchColorIcon
} from '../../../components/icons'

export type Social = {type: SocialType, url: string}
export type SocialType = 'twitter' | 'youtube' | 'twitch'
export type Donation = {tokens: string, converted: string, selected?: boolean}

export interface Props {
  balance: string
  currentAmount: string
  donationAmounts: Donation[]
  onAmountSelection: (tokens: string) => void
  id?: string
  title?: string
  name?: string
  domain: string
  bgImage?: string
  logo?: string
  social?: Social[]
  provider?: SocialType
  recurringDonation?: boolean
  children?: React.ReactNode
  onDonate: (amount: string, monthly: boolean) => void
  onClose?: () => void
  isMobile?: boolean
  logoBgColor?: CSS.Color
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

  getLogo (logo: string | undefined, domain: string) {
    return !logo
      ? <StyledLogoText isMobile={this.props.isMobile}>{(domain && domain.substring(0,1)) || ''}</StyledLogoText>
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
      default:
        return identifier
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
      isMobile
    } = this.props

    return (
      <StyledWrapper
        id={id}
        isMobile={isMobile}
        onKeyUp={this.onKeyUp}
        tabIndex={0}
      >
        <StyledBanner isMobile={isMobile}>
          <StyledClose onClick={onClose}>
            <CloseStrokeIcon />
          </StyledClose>
          <StyledBannerImage bgImage={bgImage}>
            {
              !isMobile
              ? <StyledCenter>
                  {this.getBannerTitle(name, domain, provider)}
                </StyledCenter>
              : null
            }
          </StyledBannerImage>
          <StyledContentWrapper isMobile={isMobile}>
            <StyledContent>
              <StyledLogoWrapper isMobile={isMobile}>
                <StyledLogoBorder
                  isMobile={isMobile}
                  padding={!logo}
                  bg={logoBgColor}
                >
                  {this.getLogo(logo, domain)}
                </StyledLogoBorder>
              </StyledLogoWrapper>
              <StyledTextWrapper isMobile={isMobile}>
                <StyledSocialWrapper isMobile={isMobile}>
                  {this.getSocial(social)}
                </StyledSocialWrapper>
                <StyledTitle isMobile={isMobile}>
                  {this.getTitle(title)}
                </StyledTitle>
                <StyledText isMobile={isMobile}>
                  {this.getText(children)}
                </StyledText>
              </StyledTextWrapper>
            </StyledContent>
            <StyledDonation isMobile={isMobile}>
              <StyledWallet isMobile={isMobile}>
                {getLocale('walletBalance')} <StyledTokens>{balance} BAT</StyledTokens>
              </StyledWallet>
              <Donate
                isMobile={isMobile}
                balance={parseFloat(balance)}
                donationAmounts={donationAmounts}
                title={getLocale('donationAmount')}
                onDonate={this.onDonate}
                actionText={this.state.monthly ? getLocale('doMonthly') : getLocale('sendDonation')}
                onAmountSelection={onAmountSelection}
                donateType={'big'}
                currentAmount={currentAmount}
              >
                {
                  !recurringDonation
                  ? <StyledCheckbox isMobile={isMobile}>
                      <Checkbox
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
