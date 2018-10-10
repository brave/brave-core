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
  StyledRecurring,
  StyledRemove,
  StyledWallet,
  StyledTokens,
  StyledCenter,
  StyledIconRecurringBig,
  StyledIconRemove,
  StyledSocialItem,
  StyledSocialIcon,
  StyledOption,
  StyledIconRecurring,
  StyledLogoText,
  StyledSocialWrapper
} from './style'

import Donate from '../donate/index'
import Checkbox from '../../../components/formControls/checkbox/index'
import { getLocale } from '../../../helpers'
import {
  RefreshIcon,
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
  currentAmount: number
  donationAmounts: Donation[]
  onAmountSelection: (tokens: number) => void
  id?: string
  title?: string
  domain: string
  bgImage?: string
  logo?: string
  social?: Social[]
  currentDonation?: number
  children?: React.ReactNode
  onDonate: (amount: number, monthly: boolean) => void
  onClose?: () => void
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
      ? <StyledLogoText>{(domain && domain.substring(0,1)) || ''}</StyledLogoText>
      : <img src={logo} />
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

  onDonate = (amount: number) => {
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
      children,
      title,
      currentDonation,
      balance,
      donationAmounts,
      domain,
      onAmountSelection,
      logoBgColor,
      currentAmount
    } = this.props

    return (
      <StyledWrapper id={id} onKeyUp={this.onKeyUp} tabIndex={0}>
        <StyledBanner>
          <StyledClose onClick={onClose}>
            <CloseStrokeIcon />
          </StyledClose>
          <StyledBannerImage bgImage={bgImage}>
            <StyledCenter>
              {domain}
            </StyledCenter>
          </StyledBannerImage>
          <StyledContentWrapper>
            <StyledContent>
              <StyledLogoWrapper>
                <StyledLogoBorder padding={!logo} bg={logoBgColor}>
                  {this.getLogo(logo, domain)}
                </StyledLogoBorder>
              </StyledLogoWrapper>
              <StyledTextWrapper>
                <StyledSocialWrapper>{this.getSocial(social)}</StyledSocialWrapper>
                <StyledTitle>{this.getTitle(title)}</StyledTitle>
                <StyledText>{this.getText(children)}</StyledText>
              </StyledTextWrapper>
              {
                currentDonation && !isNaN(currentDonation) && currentDonation > 0
                ? <StyledRecurring>
                  <StyledIconRecurringBig>
                    <RefreshIcon />
                  </StyledIconRecurringBig>
                  <span>{getLocale('currentDonation', { currentDonation })}</span>
                  <StyledRemove>
                    <StyledIconRemove><CloseStrokeIcon /></StyledIconRemove>{getLocale('remove')}
                  </StyledRemove>
                </StyledRecurring>
                : null
              }
            </StyledContent>
            <StyledDonation>
              <StyledWallet>
                {getLocale('walletBalance')} <StyledTokens>{balance} BAT</StyledTokens>
              </StyledWallet>
              <Donate
                balance={parseFloat(balance)}
                donationAmounts={donationAmounts}
                title={getLocale('donationAmount')}
                onDonate={this.onDonate}
                actionText={getLocale('sendDonation')}
                onAmountSelection={onAmountSelection}
                donateType={'big'}
                currentAmount={currentAmount}
              >
                <Checkbox
                  value={{ make: this.state.monthly }}
                  onChange={this.onMonthlyChange}
                  type={'dark'}
                >
                  <div data-key='make'>
                    <StyledOption>{getLocale('makeMonthly')}</StyledOption> <StyledIconRecurring><RefreshIcon /></StyledIconRecurring>
                  </div>
                </Checkbox>
              </Donate>
            </StyledDonation>
          </StyledContentWrapper>
        </StyledBanner>
      </StyledWrapper>
    )
  }
}
