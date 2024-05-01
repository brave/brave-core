/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { LocaleContext } from '../../shared/lib/locale_context'
import { lookupPublisherPlatformName } from '../../shared/lib/publisher_platform'
import { NewTabLink } from '../../shared/components/new_tab_link'

import * as style from './publisher_link.style'

interface Props {
  name: string
  url: string
  icon: string
  platform: string
  verified: boolean
  children?: React.ReactNode
}

export function PublisherLink (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const iconPath = props.verified && props.icon || props.url
  const iconSrc =
    `chrome://favicon2/?size=64&pageUrl=${encodeURIComponent(iconPath)}`
  const platformName = lookupPublisherPlatformName(props.platform)

  return (
    <style.root>
      <NewTabLink href={props.url}>
        <style.icon>
          <img src={iconSrc} />
          {
            props.verified &&
              <style.verified data-test-id='verified-icon'>
                <Icon name='verification-filled-color' />
              </style.verified>
          }
        </style.icon>
        <style.name>
          {props.name}
          {
            platformName &&
              <style.platform>
                &nbsp;{getString('on')}&nbsp;{platformName}
              </style.platform>
          }
          {props.children && <style.info>{props.children}</style.info>}
        </style.name>
      </NewTabLink>
    </style.root>
  )
}
