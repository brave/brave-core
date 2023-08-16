/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { HostContext, useHostListener } from '../lib/host_context'
import { NewTabLink } from '../../shared/components/new_tab_link'

import * as style from './alert_box.style'

export function AlertBox () {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const [earningsDisabled, setEarningsDisabled] =
    React.useState(host.state.earningsDisabled)

  useHostListener(host, (state) => {
    setEarningsDisabled(state.earningsDisabled)
  })

  if (earningsDisabled) {
    return (
      <style.alert>
        <style.alertIcon>
          <Icon name='warning-circle-filled' />
        </style.alertIcon>
        <div>
          {
            formatMessage(getString('earningsDisabledText'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='link' href='https://brave.com'>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </div>
      </style.alert>
    )
  }

  return null
}
