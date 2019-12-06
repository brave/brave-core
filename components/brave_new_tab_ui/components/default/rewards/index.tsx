/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import { convertBalance } from '../../../../brave_rewards/resources/page/utils'
import { getLocale } from '../../../../common/locale'

import {
  WidgetWrapper,
  BatIcon,
  RewardsTitle,
  Footer,
  ServiceText,
  ServiceLink,
  LearnMoreText,
  PreOptInInfo,
  Title,
  SubTitle,
  SubTitleLink,
  PreOptInAction,
  TurnOnButton,
  AmountItem,
  AmountInformation,
  AmountDescription,
  Amount,
  ConvertedAmount,
  LearnMoreLink,
  TurnOnAdsButton,
  UnsupportedMessage
} from './style'
import Notification from './notification'
import BrandedWallpaperNotification from './brandedWallpaperNotification'
import { BatColorIcon } from 'brave-ui/components/icons'

export interface RewardsProps {
  enabledAds: boolean
  enabledMain: boolean
  balance: NewTab.RewardsBalance
  promotions: NewTab.Promotion[]
  totalContribution: number
  walletCreated: boolean
  walletCreating: boolean
  walletCreateFailed: boolean
  walletCorrupted: boolean
  adsEstimatedEarnings: number
  onlyAnonWallet?: boolean
  adsSupported?: boolean
  isShowingBrandedWallpaper: boolean
  showBrandedWallpaperNotification: boolean
  brandedWallpaperData?: NewTab.BrandedWallpaper
  onCreateWallet: () => void
  onEnableAds: () => void
  onEnableRewards: () => void
  onDismissNotification: (id: string) => void
  onDisableBrandedWallpaper: () => void
}

const enum AmountItemType {
  ADS = 0,
  TIPS = 1
}

class Rewards extends React.PureComponent<RewardsProps, {}> {

  getButtonText = (isAds: boolean = false) => {
    if (isAds) {
      return getLocale('rewardsWidgetTurnOnAds')
    }

    const {
      walletCreating,
      walletCreateFailed,
      walletCorrupted
    } = this.props

    if (walletCreateFailed || walletCorrupted) {
      return getLocale('rewardsWidgetWalletFailedButton')
    }

    if (walletCreating) {
      return getLocale('rewardsWidgetTurningOn')
    }

    return getLocale('rewardsWidgetTurnOnRewards')
  }

  optInAction = (hasEnabled: boolean) => {
    if (hasEnabled) {
      this.props.onEnableRewards()
    } else {
      this.props.onCreateWallet()
    }
  }

  renderPreOptIn = () => {
    const {
      enabledMain,
      walletCreated,
      isShowingBrandedWallpaper
    } = this.props

    if (enabledMain && walletCreated) {
      return null
    }

    const hasEnabled = !enabledMain && walletCreated
    let titleText: string | JSX.Element
    let subTitleText: string | JSX.Element
    if (isShowingBrandedWallpaper) {
      titleText = getLocale('rewardsWidgetBrandedWallpaperTitle')
      const text = getLocale('rewardsWidgetBrandedWallpaperSubTitle')
      const linkStartIndex: number = text.indexOf('|')
      const linkEndIndex: number = text.lastIndexOf('|')
      const beforeLinkText = text.substring(0, linkStartIndex)
      const duringLinkText = text.substring(linkStartIndex + 1, linkEndIndex)
      const afterLinkText = text.substring(linkEndIndex + 1)
      subTitleText = (
        <>
          {beforeLinkText}
          <SubTitleLink onClick={this.props.onDisableBrandedWallpaper}>
            {duringLinkText}
          </SubTitleLink>
          {afterLinkText}
        </>
      )
    } else if (hasEnabled) {
      titleText = getLocale('rewardsWidgetReEnableTitle')
      subTitleText = getLocale('rewardsWidgetReEnableSubTitle')
    } else {
      titleText = getLocale('rewardsWidgetEnableTitle')
      subTitleText = getLocale('rewardsWidgetEnableSubTitle')
    }

    return (
      <>
        <PreOptInInfo>
          <Title>
            {titleText}
          </Title>
          <SubTitle>
            {subTitleText}
          </SubTitle>
        </PreOptInInfo>
        <PreOptInAction>
          <TurnOnButton
            data-test-id={'optInAction'}
            onClick={this.optInAction.bind(this, hasEnabled)}
          >
            {this.getButtonText()}
          </TurnOnButton>
        </PreOptInAction>
      </>
    )
  }

  renderAmountItem = (type: AmountItemType) => {
    const {
      balance,
      enabledAds,
      onEnableAds,
      adsEstimatedEarnings,
      onlyAnonWallet,
      totalContribution,
      adsSupported
    } = this.props

    const rates = balance.rates || {}
    const showEnableAds = type === AmountItemType.ADS && !enabledAds && adsSupported
    const amount = type === AmountItemType.TIPS
      ? totalContribution
      : adsEstimatedEarnings
    const converted = convertBalance(amount, rates)
    const batFormatString = onlyAnonWallet ? getLocale('rewardsWidgetBap') : getLocale('rewardsWidgetBat')

    return (
      <AmountItem isLast={type === AmountItemType.TIPS}>
        <AmountDescription>
          {
            type === AmountItemType.ADS
            ? getLocale('rewardsWidgetEstimatedEarnings')
            : getLocale('rewardsWidgetMonthlyTips')
          }
        </AmountDescription>
        {
          showEnableAds
          ? <TurnOnAdsButton onClick={onEnableAds}>
              {this.getButtonText(true)}
            </TurnOnAdsButton>
          : null
        }
        {
          !showEnableAds && !(type === AmountItemType.ADS && !adsSupported)
          ? <AmountInformation data-test-id={`widget-amount-total-${type}`}>
              <Amount>{amount.toFixed(1)}</Amount>
              <ConvertedAmount>
                {`${batFormatString} ${converted} USD`}
              </ConvertedAmount>
            </AmountInformation>
          : null
        }
        {
          type === AmountItemType.ADS && !adsSupported
          ? <UnsupportedMessage>
              {getLocale('rewardsWidgetAdsNotSupported')}
            </UnsupportedMessage>
          : null
        }
      </AmountItem>
    )
  }

  renderRewardsInfo = () => {
    const {
      enabledMain,
      walletCreated
    } = this.props

    if (!enabledMain || !walletCreated) {
      return null
    }

    return (
      <div data-test-id2={'enableMain'}>
        {this.renderAmountItem(AmountItemType.ADS)}
        {this.renderAmountItem(AmountItemType.TIPS)}
      </div>
    )
  }

  renderLearnMore = () => {
    return (
      <LearnMoreText>
        <LearnMoreLink target={'_blank'} href={'chrome://rewards'}>
          {getLocale('learnMore')}
        </LearnMoreLink>
        {getLocale('rewardsWidgetAboutRewards')}
      </LearnMoreText>
    )
  }

  renderPrivacyPolicy = () => {
    return (
      <>
        <ServiceText>
          {getLocale('rewardsWidgetServiceText')} <ServiceLink target={'_blank'} href={'https://brave.com/terms-of-use'}>{getLocale('rewardsWidgetTermsOfService')}</ServiceLink> {getLocale('rewardsWidgetAnd')} <ServiceLink target={'_blank'} href={'https://brave.com/privacy#rewards'}>{getLocale('rewardsWidgetPrivacyPolicy')}</ServiceLink>.
        </ServiceText>
      </>
    )
  }

  renderNotifications = () => {
    let { promotions, onDismissNotification } = this.props

    // Uncomment for demo promotion notifications data:
    //
    // const showDummyPromotion = true
    // if (showDummyPromotion) {
    //   promotions = [{
    //     type: 1,
    //     promotionId: '1234'
    //   }]
    // }

    return (
      <>
        {promotions && promotions.map((promotion: NewTab.Promotion, index) => {
          return (
            <Notification
              promotion={promotion}
              key={`notification-${index}`}
              onDismissNotification={onDismissNotification}
              order={index + 1}
            />
          )
        })}
        { this.props.showBrandedWallpaperNotification &&
        <BrandedWallpaperNotification
          onDismissNotification={onDismissNotification}
          brandedWallpaperData={this.props.brandedWallpaperData}
          onNotificationAction={this.props.onDisableBrandedWallpaper}
          order={promotions ? promotions.length + 1 : 1}
        />
        }
      </>
    )
  }

  render () {
    const {
      enabledMain,
      walletCreated
    } = this.props

    return (
      <WidgetWrapper>
        <BatIcon>
          <BatColorIcon />
        </BatIcon>
        <RewardsTitle>
          {getLocale('rewardsWidgetBraveRewards')}
        </RewardsTitle>
        {this.renderPreOptIn()}
        {this.renderRewardsInfo()}
        <Footer>
          {
            enabledMain && walletCreated
            ? this.renderLearnMore()
            : this.renderPrivacyPolicy()
          }
        </Footer>
        {
          enabledMain
          ? this.renderNotifications()
          : null
        }
      </WidgetWrapper>
    )
  }
}

export const RewardsWidget = createWidget(Rewards)
