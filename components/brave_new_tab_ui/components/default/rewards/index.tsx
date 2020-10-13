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
  ServiceLink,
  LearnMoreText,
  AmountItem,
  AmountDescription,
  Amount,
  ConvertedAmount,
  AmountUSD,
  TurnOnAdsButton,
  UnsupportedMessage,
  TurnOnText,
  StyledTOS
} from './style'
import { StyledTitleTab } from '../widgetTitleTab'
import Notification from './notification'
import BrandedWallpaperNotification from './brandedWallpaperNotification'
import { BatColorIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

export interface RewardsProps {
  enabledAds: boolean
  balance: NewTab.RewardsBalance
  parameters: NewTab.RewardsParameters
  promotions: NewTab.Promotion[]
  totalContribution: number
  adsEstimatedEarnings: number
  onlyAnonWallet?: boolean
  adsSupported?: boolean
  isShowingBrandedWallpaper: boolean
  isNotification?: boolean
  showBrandedWallpaperNotification: boolean
  brandedWallpaperData?: NewTab.BrandedWallpaper
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
  onEnableAds: () => void
  onDismissNotification: (id: string) => void
  onDismissBrandedWallpaperNotification: (isUserAction: boolean) => void
  onDisableBrandedWallpaper: () => void
}

class Rewards extends React.PureComponent<RewardsProps, {}> {
  renderAmountItem = () => {
    const {
      parameters,
      enabledAds,
      onEnableAds,
      adsEstimatedEarnings,
      onlyAnonWallet,
      adsSupported
    } = this.props

    const rate = parameters.rate || 0.0
    const showEnableAds = !enabledAds && adsSupported
    const amount = adsEstimatedEarnings
    const converted = convertBalance(amount, rate)
    const batFormatString = onlyAnonWallet ? getLocale('rewardsWidgetBap') : getLocale('rewardsWidgetBat')

    return (
      <AmountItem isActionPrompt={!!showEnableAds} isLast={false}>
        {
          showEnableAds
          ? <>
            <TurnOnText>
              {getLocale('rewardsWidgetTurnOnText')}
            </TurnOnText>
            <TurnOnAdsButton
              onClick={onEnableAds}
              type={'accent'}
              brand={'rewards'}
              text={getLocale('rewardsWidgetTurnOnAds')}
            />
          </>
          : null
        }
        {
          !showEnableAds && adsSupported
          ? <div data-test-id={`widget-amount-total-ads`}>
              <Amount>{amount.toFixed(3)}</Amount>
              <ConvertedAmount>
                {batFormatString}<AmountUSD>{converted} USD</AmountUSD>
              </ConvertedAmount>
            </div>
          : null
        }
        {
          !adsSupported
          ? <UnsupportedMessage>
              {getLocale('rewardsWidgetAdsNotSupported')}
            </UnsupportedMessage>
          : null
        }
        <AmountDescription>
          {
            showEnableAds
            ? <StyledTOS title={getLocale('rewardsWidgetEarnAndGive')} />
            : getLocale('rewardsWidgetEstimatedEarnings')
          }
        </AmountDescription>
      </AmountItem>
    )
  }

  renderTipsBox = () => {
    const {
      parameters,
      onlyAnonWallet,
      totalContribution
    } = this.props

    const rate = parameters.rate || 0.0
    const amount = totalContribution
    const converted = convertBalance(amount, rate)
    const batFormatString = onlyAnonWallet ? getLocale('rewardsWidgetBap') : getLocale('rewardsWidgetBat')

    return (
      <AmountItem isLast={true}>
        <div data-test-id={`widget-amount-total-tips`}>
          <Amount>{amount.toFixed(3)}</Amount>
          <ConvertedAmount>
            {batFormatString}<AmountUSD>{converted} USD</AmountUSD>
          </ConvertedAmount>
        </div>
        <AmountDescription>
          {getLocale('rewardsWidgetMonthlyTips')}
        </AmountDescription>
      </AmountItem>
    )
  }

  renderRewardsInfo = () => {
    const {
      adsSupported
    } = this.props

    return (
      <div>
        {adsSupported && this.renderAmountItem()}
        {this.renderTipsBox()}
      </div>
    )
  }

  renderLearnMore = () => {
    const text = getLocale('rewardsWidgetAboutRewards')
    const { beforeTag, duringTag, afterTag } = splitStringForTag(text)
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

  renderNotifications = (singleOrphaned = false) => {
    let {
      promotions,
      onDismissNotification,
      enabledAds,
      onEnableAds
    } = this.props

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
          onDismissNotification={this.dismissBrandedWallpapernotificationUserAction}
          onEnableAds={enabledAds ? undefined : onEnableAds}
          brandedWallpaperData={this.props.brandedWallpaperData}
          onHideSponsoredImages={this.props.onDisableBrandedWallpaper}
          order={promotions ? promotions.length + 1 : 1}
        />
        }
      </Wrapper>
    )
  }

  renderTitle = () => {
    const { showContent } = this.props

    return (
      <RewardsTitle isInTab={!showContent}>
        <BatIcon>
          <BatColorIcon />
        </BatIcon>
        {getLocale('rewardsWidgetBraveRewards')}
      </RewardsTitle>
    )
  }

  renderTitleTab = () => {
    const { onShowContent, stackPosition } = this.props

    return (
      <StyledTitleTab onClick={onShowContent} stackPosition={stackPosition}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  dismissBrandedWallpapernotificationUserAction = () => {
    this.props.onDismissBrandedWallpaperNotification(true)
  }

  render () {
    const {
      isNotification,
      showContent
    } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    // Handle isNotification:
    //   - if rewards isn't on, we ourselves are a notification
    //   - if rewards is on, only show a single notification (the last one)
    //     (intended for branded wallpaper notification).
    if (isNotification) {
      return this.renderNotifications(true)
    }

    return (
      <WidgetWrapper>
        <WidgetLayer>
          {isNotification &&
          <CloseIcon onClick={this.dismissBrandedWallpapernotificationUserAction}>
            <CloseStrokeIcon />
          </CloseIcon>
          }
          {this.renderTitle()}
          {this.renderRewardsInfo()}
          <Footer>
            {this.renderLearnMore()}
          </Footer>
        </WidgetLayer>
        {this.renderNotifications()}
      </WidgetWrapper>
    )
  }
}

export const RewardsWidget = createWidget(Rewards)
