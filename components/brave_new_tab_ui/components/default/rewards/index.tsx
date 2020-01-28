/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import { convertBalance } from '../../../../brave_rewards/resources/page/utils'
import { getLocale, splitStringForTag } from '../../../../common/locale'

import {
  WidgetWrapper,
  WidgetLayer,
  NotificationsList,
  BatIcon,
  CloseIcon,
  RewardsTitle,
  Footer,
  ServiceText,
  ServiceLink,
  LearnMoreText,
  PreOptInInfo,
  Title,
  LearnMoreTextButton,
  SubTitle,
  SubTitleLink,
  PreOptInAction,
  TurnOnButton,
  AmountItem,
  AmountDescription,
  Amount,
  ConvertedAmount,
  AmountUSD,
  TurnOnAdsButton,
  UnsupportedMessage
} from './style'
import Notification from './notification'
import BrandedWallpaperNotification from './brandedWallpaperNotification'
import { BatColorIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

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
  isNotification?: boolean
  showBrandedWallpaperNotification: boolean
  brandedWallpaperData?: NewTab.BrandedWallpaper
  onCreateWallet: () => void
  onEnableAds: () => void
  onEnableRewards: () => void
  onDismissNotification: (id: string) => void
  onDismissBrandedWallpaperNotification: () => void
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
      titleText = getLocale('rewardsWidgetEnableBrandedWallpaperTitle')
      const text = getLocale('rewardsWidgetEnableBrandedWallpaperSubTitle')
        .replace('$3', 'Brave Rewards')
      const { beforeTag, duringTag, afterTag } = splitStringForTag(text, '$1', '$2')
      subTitleText = (
        <>
          {beforeTag}
          <SubTitleLink onClick={this.props.onDisableBrandedWallpaper}>
            {duringTag}
          </SubTitleLink>
          {afterTag}
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
          <LearnMoreTextButton href='https://www.brave.com/brave-rewards'>
            {getLocale('rewardsWidgetTurnOnLearnMore')}
          </LearnMoreTextButton>
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
      <AmountItem isActionPrompt={!!showEnableAds} isLast={type === AmountItemType.TIPS}>
        {
          showEnableAds
          ? <TurnOnAdsButton onClick={onEnableAds}>
              {this.getButtonText(true)}
            </TurnOnAdsButton>
          : null
        }
        {
          !showEnableAds && !(type === AmountItemType.ADS && !adsSupported)
          ? <div data-test-id={`widget-amount-total-${type}`}>
              <Amount>{amount.toFixed(1)}</Amount>
              <ConvertedAmount>
                {batFormatString}<AmountUSD>{converted} USD</AmountUSD>
              </ConvertedAmount>
            </div>
          : null
        }
        {
          type === AmountItemType.ADS && !adsSupported
          ? <UnsupportedMessage>
              {getLocale('rewardsWidgetAdsNotSupported')}
            </UnsupportedMessage>
          : null
        }
        <AmountDescription>
          {
            type === AmountItemType.ADS
            ? showEnableAds
                ? getLocale('rewardsWidgetAdsOptInDescription')
                : getLocale('rewardsWidgetEstimatedEarnings')
            : getLocale('rewardsWidgetMonthlyTips')
          }
        </AmountDescription>
      </AmountItem>
    )
  }

  renderRewardsInfo = () => {
    const {
      enabledMain,
      walletCreated,
      adsSupported
    } = this.props

    if (!enabledMain || !walletCreated) {
      return null
    }

    return (
      <div data-test-id2={'enableMain'}>
        {adsSupported && this.renderAmountItem(AmountItemType.ADS)}
        {this.renderAmountItem(AmountItemType.TIPS)}
      </div>
    )
  }

  renderLearnMore = () => {
    const text = getLocale('rewardsWidgetAboutRewards')
    const { beforeTag, duringTag, afterTag } = splitStringForTag(text, '$1', '$2')
    return (
      <LearnMoreText>
        {beforeTag}
        <ServiceLink href={'chrome://rewards'}>
          {duringTag}
        </ServiceLink>
        {afterTag}
      </LearnMoreText>
    )
  }

  renderPrivacyPolicy = () => {
    const text = getLocale('rewardsWidgetServiceText')
    const { beforeTag, duringTag, afterTag } = splitStringForTag(text, '$1', '$2')
    return (
      <>
        <ServiceText>
          {beforeTag}<ServiceLink href={'https://brave.com/terms-of-use'}>{duringTag}</ServiceLink>{afterTag}
        </ServiceText>
      </>
    )
  }

  renderNotifications = (singleOrphaned = false) => {
    let {
      promotions,
      onDismissNotification,
      enabledAds,
      onEnableAds
    } = this.props

    // Uncomment for demo promotion notifications data:
    //
    // const showDummyPromotion = true
    // if (showDummyPromotion) {
    //   promotions = [{
    //     type: 1,
    //     promotionId: '1234'
    //   }]
    // }

    // TODO(petemill): If we want a true 'single' mode then
    // only show a single notification from any source.
    // For now, this only happens for the branded wallpaper notification.
    promotions = singleOrphaned ? [] : (promotions || [])
    const Wrapper = singleOrphaned ? React.Fragment : NotificationsList
    return (
      <Wrapper>
        {promotions.map((promotion: NewTab.Promotion, index) => {
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
          isOrphan={singleOrphaned}
          onDismissNotification={this.props.onDismissBrandedWallpaperNotification}
          onEnableAds={enabledAds ? undefined : onEnableAds}
          brandedWallpaperData={this.props.brandedWallpaperData}
          onHideSponsoredImages={this.props.onDisableBrandedWallpaper}
          order={promotions ? promotions.length + 1 : 1}
        />
        }
      </Wrapper>
    )
  }

  dismissNotification (notificationType: string) {
    this.props.onDismissNotification(notificationType)
  }

  render () {
    const {
      enabledMain,
      walletCreated,
      isNotification
    } = this.props

    // Handle isNotification:
    //   - if rewards isn't on, we ourselves are a notification
    //   - if rewards is on, only show a single notification (the last one)
    //     (intended for branded wallpaper notification).
    if (isNotification) {
      if (enabledMain) {
        return this.renderNotifications(true)
      }
    }

    return (
      <WidgetWrapper isEnabled={enabledMain}>
        <WidgetLayer>
          {isNotification &&
          <CloseIcon onClick={this.props.onDismissBrandedWallpaperNotification}>
            <CloseStrokeIcon />
          </CloseIcon>
          }
          <RewardsTitle>
            <BatIcon>
              <BatColorIcon />
            </BatIcon>
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
        </WidgetLayer>
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
