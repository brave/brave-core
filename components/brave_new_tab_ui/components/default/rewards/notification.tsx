/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  Title,
  SubTitle,
  CloseIcon,
  Content,
  NotificationWrapper,
  NotificationButton
} from './style'
import { CloseStrokeIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'

interface NotificationProps {
  promotion: NewTab.Promotion
  onDismissNotification: (id: string) => void
}

export default class RewardsNotification extends React.PureComponent<NotificationProps, {}> {

  dismissNotification = () => {
    this.props.onDismissNotification(this.props.promotion.promotionId)
  }

  onNotificationAction = () => {
    this.dismissNotification()
    chrome.braveRewards.openBrowserActionUI(`brave_rewards_panel.html#grant_${this.props.promotion.promotionId}`)
  }

  render () {
    return (
      <NotificationWrapper>
        <CloseIcon onClick={this.dismissNotification}>
          <CloseStrokeIcon />
        </CloseIcon>
        <Content>
          <Title>
            {getLocale('rewardsWidgetNotificationTitle')}
          </Title>
          <SubTitle>
            {
              this.props.promotion.type === 1
              ? getLocale('rewardsWidgetNotificationTextAds')
              : getLocale('rewardsWidgetNotificationTextUGP')
            }
          </SubTitle>
          <NotificationButton onClick={this.onNotificationAction}>
            {getLocale('rewardsWidgetClaimMyRewards')}
          </NotificationButton>
        </Content>
      </NotificationWrapper>
    )
  }
}
