/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { BrowserName } from '../lib/import_profile_helper'

interface Props {
  name: BrowserName | null
}

export function BrowserIcon(props: Props) {
  switch (props.name) {
    case 'Google Chrome':
      return <Icon name='chromerelease-color' />
    case 'Google Chrome Beta':
      return <Icon name='chromebeta-color' />
    case 'Google Chrome Canary':
      return <Icon name='chromecanary-color' />
    case 'Google Chrome Dev':
      return <Icon name='chromedev-color' />
    case 'Brave':
      return <Icon name='social-brave-release-favicon-fullheight-color' />
    case 'Mozilla Firefox':
      return <Icon name='firefox-color' />
    case 'Microsoft Edge':
    case 'Microsoft Internet Explorer':
      return <Icon name='edge-color' />
    case 'NAVER Whale':
      return <Icon name='naver-color' />
    case 'Opera':
      return <Icon name='opera-color' />
    case 'Safari':
      return <Icon name='safari-color' />
    case 'Vivaldi':
      return <Icon name='vivaldi-color' />
    case 'Yandex':
      return <Icon name='yandex-color' />
    case 'Chromium':
    case null:
      return <Icon name='chromium-color' />
  }
}
