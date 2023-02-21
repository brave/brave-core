/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../lib/locale_context'
import { UserType } from '../lib/user_type'
import { NewTabLink } from './new_tab_link'
import { AlertIcon } from './icons/alert_icon'
import { ArrowNextIcon } from './icons/arrow_next_icon'
import { CloseIcon } from './icons/close_icon'

import * as urls from '../lib/rewards_urls'
import * as style from './vbat_notice.style'

const dateFormatter = new Intl.DateTimeFormat(undefined, {
  year: 'numeric',
  month: 'long',
  day: 'numeric',
  hour: 'numeric',
  minute: 'numeric'
})

export function shouldShowVBATNotice (
  userType: UserType,
  vbatDeadline: number | undefined
) {
  return (
    userType === 'legacy-unconnected' &&
    vbatDeadline &&
    vbatDeadline > Date.now()
  )
}

interface Props {
  vbatDeadline: number | undefined
  canConnectAccount: boolean
  declaredCountry: string
  onConnectAccount: () => void
  onClose?: () => void
}

export function VBATNotice (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function getMessages () {
    if (!props.vbatDeadline) {
      return null
    }

    const deadline = new Date(props.vbatDeadline)

    if (!props.canConnectAccount) {
      return {
        title: getString('rewardsVBATNoticeTitle2'),
        text: formatMessage(getString('rewardsVBATNoticeText2'), [
          dateFormatter.format(deadline),
          props.declaredCountry
        ])
      }
    }

    return {
      title: getString('rewardsVBATNoticeTitle1'),
      text: formatMessage(getString('rewardsVBATNoticeText1'), [
        dateFormatter.format(deadline)
      ])
    }
  }

  const messages = getMessages()
  if (!messages) {
    return null
  }

  return (
    <style.root>
      <style.header className='vbat-notice-header'>
        <style.headerIcon><AlertIcon /></style.headerIcon>
        {
          props.onClose &&
            <style.headerClose className='vbat-notice-close'>
              <button onClick={props.onClose}><CloseIcon /></button>
            </style.headerClose>
        }
        <style.headerText>{messages.title}</style.headerText>
      </style.header>
      <style.content className='vbat-notice-content'>
        {messages.text}
        <style.actions className='vbat-notice-actions'>
          {
            props.canConnectAccount &&
              <style.connectAction className='vbat-notice-connect'>
                <button onClick={props.onConnectAccount}>
                  {getString('rewardsConnectAccount')}<ArrowNextIcon />
                </button>
              </style.connectAction>
          }
          <style.learnMoreAction>
            <NewTabLink href={urls.rewardsChangesURL}>
              {getString('rewardsLearnMore')}
            </NewTabLink>
          </style.learnMoreAction>
        </style.actions>
      </style.content>
    </style.root>
  )
}
