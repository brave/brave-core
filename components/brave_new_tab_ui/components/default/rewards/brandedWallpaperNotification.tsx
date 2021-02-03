/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  Title,
  SubTitle,
  SubTitleLink,
  CloseIcon,
  NotificationWrapper,
  OrphanedNotificationWrapper,
  NotificationAction,
  TurnOnAdsButton,
  StyledTOS,
  StyleCenter
} from './style'
import { CloseStrokeIcon } from 'brave-ui/components/icons'
import { getLocale, splitStringForTag } from '../../../../common/locale'

interface NotificationProps {
  onDismissNotification: () => void
  brandedWallpaperData?: NewTab.BrandedWallpaper
  isOrphan?: boolean
  onStartRewards?: () => void
  onHideSponsoredImages: () => void
  order: number
}

export default class BrandedWallpaperRewardsNotification extends React.PureComponent<NotificationProps, {}> {

  renderPostAdsOptInContent () {
    const text = getLocale('rewardsWidgetBrandedNotificationDescription')
    const { beforeTag, duringTag, afterTag } = splitStringForTag(text)
    return (
      <StyleCenter>
        <Title>
          {getLocale('rewardsWidgetBrandedNotificationTitle')}
        </Title>
        <SubTitle>
          {beforeTag}
          <SubTitleLink href='https://brave.com/brave-rewards/'>
            {duringTag}
          </SubTitleLink>
          {afterTag}
        </SubTitle>
        <NotificationAction onClick={this.props.onHideSponsoredImages}>
          {getLocale('rewardsWidgetBrandedNotificationHideAction')}
        </NotificationAction>
      </StyleCenter>
    )
  }

  renderPreAdsOptInContent () {
    const text = getLocale('rewardsWidgetEnableBrandedWallpaperSubTitle')
    const { beforeTag, duringTag, afterTag } = splitStringForTag(text)
    return (
      <StyleCenter>
        <Title>
          {getLocale('rewardsWidgetEnableBrandedWallpaperTitle')}
        </Title>
        <SubTitle>
          {beforeTag}
          <SubTitleLink onClick={this.props.onHideSponsoredImages}>
            {duringTag}
          </SubTitleLink>
          {afterTag}
        </SubTitle>
        <TurnOnAdsButton
          onClick={this.props.onStartRewards}
          type={'accent'}
          brand={'rewards'}
          text={getLocale('rewardsWidgetTurnOnAds')}
        />
        <StyledTOS title={getLocale('rewardsWidgetTurnOnAds')} />
      </StyleCenter>
    )
  }

  render () {
    const { brandedWallpaperData, isOrphan } = this.props
    if (!brandedWallpaperData) {
      console.error('Asked to render a branded wallpaper but there was no data!')
      return null
    }
    const styleVars = { '--notification-counter': this.props.order } as React.CSSProperties
    const Wrapper = isOrphan ? OrphanedNotificationWrapper : NotificationWrapper
    return (
      <Wrapper
        style={styleVars}
      >
        <CloseIcon onClick={this.props.onDismissNotification}>
          <CloseStrokeIcon />
        </CloseIcon>
          { this.props.onStartRewards
              ? this.renderPreAdsOptInContent()
              : this.renderPostAdsOptInContent()
          }
      </Wrapper>
    )
  }
}
