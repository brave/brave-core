/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useLocaleContext } from '../lib/locale_strings'
import { AppModelContext, useAppState } from '../lib/app_model_context'
import { TabOpenerContext } from '../../shared/components/new_tab_link'
import { Modal } from './modal'

import {
  getNotificationView,
  Notification,
  NotificationActionViewProps,
  NotificationBodyProps,
  NotificationTitleProps,
  OpenLinkAction
} from '../../shared/components/notifications'

import { style } from './notification_modal.style'

function Title(props: NotificationTitleProps) {
  function renderIcon () {
    switch (props.style) {
      case 'funding': return <Icon name='hand-coins' />
      case 'error': return <Icon name='warning-circle-filled' />
      default: return <Icon name='info-filled' />
    }
  }

  return (
    <div className={`title ${props.style || 'information'}`}>
      {renderIcon()}
      <h3>{props.children}</h3>
    </div>
  )
}

function Body(props: NotificationBodyProps) {
  return <p>{props.children}</p>
}

function Action(props: NotificationActionViewProps) {
  const { getString } = useLocaleContext()
  const model = React.useContext(AppModelContext)
  const tabOpener = React.useContext(TabOpenerContext)
  const [externalWallet] = useAppState((state) => [state.externalWallet])

  function onActionClick () {
    model.clearNotification(props.notification.id)
    if (props.action) {
      switch (props.action.type) {
        case 'open-link':
          tabOpener.openTab((props.action as OpenLinkAction).url)
          break
        case 'reconnect-external-wallet':
          if (externalWallet && !externalWallet.authenticated) {
            model.beginExternalWalletLogin(externalWallet.provider)
          }
          break
      }
    }
  }

  return (
    <div className='action'>
      <Button onClick={onActionClick}>
        {props.label || getString('closeButtonLabel')}
      </Button>
    </div>
  )
}

interface Props {
  notification: Notification
}

export function NotificationModal(props: Props) {
  const model = React.useContext(AppModelContext)
  const View = getNotificationView(props.notification)

  function dismiss() {
    model.clearNotification(props.notification.id)
  }

  return (
    <Modal onEscape={dismiss}>
      <div {...style}>
        <Modal.Header onClose={dismiss} />
        <View
          notification={props.notification}
          Title={Title}
          Body={Body}
          Action={Action}
        />
      </div>
    </Modal>
  )
}
