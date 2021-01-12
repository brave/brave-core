/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  Title,
  NotificationCloseIcon,
  OrphanedNotificationWrapper,
  NotificationAction,
  StartRewardsButton,
  BatIcon,
  RewardsNotificationTitle,
  NotificationContent,
  SubAction,
  NotificationTOS
} from './style'
import { BatColorIcon, CloseStrokeIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'

interface NotificationProps {
  onDismissNotification: () => void
  brandedWallpaperData?: NewTab.BrandedWallpaper
  onStartRewards?: () => void
  onHideSponsoredImages: () => void
}

export default class BrandedWallpaperRewardsNotification extends React.PureComponent<NotificationProps, {}> {

  renderHeader () {
    return (
      <RewardsNotificationTitle>
        <BatIcon>
          <BatColorIcon />
        </BatIcon>
        {getLocale('rewardsWidgetBraveRewards')}
        <NotificationCloseIcon onClick={this.props.onDismissNotification}>
          <CloseStrokeIcon />
        </NotificationCloseIcon>
      </RewardsNotificationTitle>
    )
  }

  renderFooter () {
    return null
  }

  renderPostAdsOptInContent () {
    return (
      <NotificationContent>
        <Title>
          {'You’re earning tokens for viewing this sponsored image.'}
        </Title>
        <SubAction>
          <NotificationAction href='https://brave.com/brave-rewards/'>
            {'Learn More'}
          </NotificationAction>
        </SubAction>
        <NotificationAction onClick={this.props.onHideSponsoredImages}>
          {getLocale('rewardsWidgetBrandedNotificationHideAction')}
        </NotificationAction>
      </NotificationContent>
    )
  }

  renderPreAdsOptInContent () {
    return (
      <NotificationContent>
        <Title>
          {'Earn tokens for viewing this image and support content creators.'}
        </Title>
        <StartRewardsButton onClick={this.props.onStartRewards}>
         {getLocale('rewardsWidgetTurnOnAds')}
        </StartRewardsButton>
        <NotificationTOS>
          {'By clicking Rewards, you agree to the '}<NotificationAction href='https://brave.com/brave-rewards/'>{'Terms of Service'}</NotificationAction> {' and '} <NotificationAction href='https://brave.com/brave-rewards/'>{'Privacy Policy'}</NotificationAction>
        </NotificationTOS>
        <SubAction>
          <NotificationAction href='https://brave.com/brave-rewards/'>
            {'Learn More'}
          </NotificationAction>
        </SubAction>
        <NotificationAction onClick={this.props.onHideSponsoredImages}>
          {getLocale('rewardsWidgetBrandedNotificationHideAction')}
        </NotificationAction>
      </NotificationContent>
    )
  }

  render () {
    const { brandedWallpaperData } = this.props
    if (!brandedWallpaperData) {
      console.error('Asked to render a branded wallpaper but there was no data!')
      return null
    }
    const styleVars = { '--notification-counter': 0 } as React.CSSProperties
    return (
      <OrphanedNotificationWrapper
        style={styleVars}
      >
        {this.renderHeader()}
          { this.props.onStartRewards
              ? this.renderPreAdsOptInContent()
              : this.renderPostAdsOptInContent()
          }
      </OrphanedNotificationWrapper>
    )
  }
}
