/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  Title,
  SubTitle,
  SubTitleLink,
  CloseIcon,
  Content,
  NotificationWrapper,
  NotificationAction
} from './style'
import { CloseStrokeIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'

interface NotificationProps {
  onDismissNotification: (id: string) => void
  brandedWallpaperData?: NewTab.BrandedWallpaper
  onNotificationAction: () => void
  order: number
}

export default class RewardsNotification extends React.PureComponent<NotificationProps, {}> {

  dismissNotification = () => {
    this.props.onDismissNotification('brandedWallpaper')
  }

  render () {
    const { brandedWallpaperData } = this.props
    if (!brandedWallpaperData) {
      console.error('Asked to render a branded wallpaper but there was no data!')
      return null
    }

    const text = getLocale('rewardsWidgetBrandedNotificationText')
    const companyNameIndex: number = text.indexOf('$1')
    const beforeLinkText = text.substring(0, companyNameIndex)
    const afterLinkText = text.substring(companyNameIndex + 2)
    const subTitleText = (
      <>
        {beforeLinkText}
        <SubTitleLink href={brandedWallpaperData.logo.destinationUrl}>
          {brandedWallpaperData.logo.companyName}
        </SubTitleLink>
        {afterLinkText}
      </>
    )

    const styleVars = { '--notification-counter': this.props.order } as React.CSSProperties

    return (
      <NotificationWrapper
        style={styleVars}
      >
        <CloseIcon onClick={this.dismissNotification}>
          <CloseStrokeIcon />
        </CloseIcon>
        <Content>
          <Title>
            {getLocale('rewardsWidgetBrandedNotificationTitle')}
          </Title>
          <SubTitle>
            {subTitleText}
          </SubTitle>
          <NotificationAction onClick={this.props.onNotificationAction}>
            {getLocale('rewardsWidgetBrandedNotificationAction')}
          </NotificationAction>
        </Content>
      </NotificationWrapper>
    )
  }
}
