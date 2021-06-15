/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import { convertBalance } from '../../../../brave_rewards/resources/page/utils'
import { getLocale, splitStringForTag } from '../../../../common/locale'

import {
  ArrivingSoon,
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
  TurnOnTitle,
  StyledTOS,
  StyleCenter
} from './style'
import { StyledTitleTab } from '../widgetTitleTab'
import Notification from './notification'
import BrandedWallpaperNotification from './brandedWallpaperNotification'
import { BatColorIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

import { formatMessage } from '../../../../../components/brave_rewards/resources/shared/lib/locale_context'
import { getDaysUntilRewardsPayment } from '../../../../../components/brave_rewards/resources/shared/lib/pending_rewards'

export interface RewardsProps {
  enabledAds: boolean
  balance: NewTab.RewardsBalance
  adsAccountStatement: NewTab.AdsAccountStatement
  parameters: NewTab.RewardsParameters
  promotions: NewTab.Promotion[]
  totalContribution: number
  adsSupported?: boolean
  isShowingBrandedWallpaper: boolean
  isNotification?: boolean
  showBrandedWallpaperNotification: boolean
  brandedWallpaperData?: NewTab.BrandedWallpaper
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
  onStartRewards: () => void
  onDismissNotification: (id: string) => void
  onDismissBrandedWallpaperNotification: (isUserAction: boolean) => void
  onDisableBrandedWallpaper: () => void
}

class Rewards extends React.PureComponent<RewardsProps, {}> {
  renderAmountItem = () => {
    const {
      parameters,
      enabledAds,
      adsAccountStatement,
      adsSupported
    } = this.props

    const rate = parameters.rate || 0.0
    const showEnableAds = !enabledAds && adsSupported
    const amount = adsAccountStatement ? adsAccountStatement.earningsThisMonth : 0
    const converted = convertBalance(amount, rate)
    const batFormatString = getLocale('rewardsWidgetBat')

    return (
      <AmountItem isActionPrompt={!!showEnableAds} isLast={false}>
        {this.renderPendingRewardsNotice()}
        {
          adsSupported
          ? <div data-test-id={`widget-amount-total-ads`}>
              <Amount>{amount.toFixed(3)}</Amount>
              <ConvertedAmount>
                {batFormatString}<AmountUSD>{converted} USD</AmountUSD>
              </ConvertedAmount>
            </div>
          :
            <UnsupportedMessage>
              {getLocale('rewardsWidgetAdsNotSupported')}
            </UnsupportedMessage>
        }
        <AmountDescription>
          {getLocale('rewardsWidgetEstimatedEarnings')}
        </AmountDescription>
      </AmountItem>
    )
  }

  renderTipsBox = () => {
    const {
      enabledAds,
      parameters,
      totalContribution
    } = this.props

    if (!enabledAds) {
      return null
    }

    const rate = parameters.rate || 0.0
    const amount = totalContribution
    const converted = convertBalance(amount, rate)
    const batFormatString = getLocale('rewardsWidgetBat')

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

  renderOptIn = () => {
    const { onStartRewards } = this.props

    return (
      <StyleCenter>
        <TurnOnTitle>
          {getLocale('rewardsWidgetTurnOnTitle')}
        </TurnOnTitle>
        <TurnOnText>
          {getLocale('rewardsWidgetTurnOnText')}
        </TurnOnText>
        <TurnOnAdsButton
          onClick={onStartRewards}
          type={'accent'}
          brand={'rewards'}
          text={getLocale('rewardsWidgetTurnOnAds')}
        />
        <StyledTOS title={getLocale('rewardsWidgetTurnOnAds')} />
      </StyleCenter>
    )
  }

  renderPendingRewardsNotice = () => {
    if (!this.props.adsAccountStatement) {
      return null
    }

    const {
      nextPaymentDate,
      earningsLastMonth
    } = this.props.adsAccountStatement

    if (earningsLastMonth <= 0) {
      return null
    }

    const days = getDaysUntilRewardsPayment(nextPaymentDate)
    if (!days) {
      return null
    }

    return (
      <ArrivingSoon>
        {
          formatMessage(getLocale('pendingRewardsMessage'), [
            <span className='amount' key='amount'>
              <strong>+{earningsLastMonth}</strong> BAT
            </span>,
            days
          ])
        }
      </ArrivingSoon>
    )
  }

  renderRewardsInfo = () => {
    const {
      adsSupported,
      enabledAds
    } = this.props

    if (!enabledAds && adsSupported) {
      return this.renderOptIn()
    }

    return (
      <div>
        {this.renderAmountItem()}
        {this.renderTipsBox()}
        <Footer>
          {this.renderLearnMore()}
        </Footer>
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
      onStartRewards
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
          onStartRewards={enabledAds ? undefined : onStartRewards}
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
        </WidgetLayer>
        {this.renderNotifications()}
      </WidgetWrapper>
    )
  }
}

export const RewardsWidget = createWidget(Rewards)
