/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { SearchEngineInfo } from '../../models/search_model'
import { PcdnImage } from '../pcdn_image'

function getNamedIcon(engineHost: string) {
  switch (engineHost) {
    case 'google.com':
      return 'google-color'
    case 'duckduckgo.com':
      return 'duckduckgo-color'
    case 'search.brave.com':
      return 'social-brave-release-favicon-fullheight-color'
    case 'www.bing.com':
      return 'bing-color'
    case 'www.qwant.com':
      return 'qwant-color'
    case 'www.startpage.com':
      return 'startpage-color'
    case 'search.yahoo.com':
      return 'yahoo-color'
    case 'yandex.com':
      return 'yandex-color'
    case 'www.ecosia.org':
      return 'ecosia-color'
  }
  return ''
}

interface Props {
  engine: SearchEngineInfo
}

export function EngineIcon(props: Props) {
  const { engine } = props
  const iconName = getNamedIcon(engine.host)
  if (iconName) {
    return <Icon name={iconName} className='engine-icon' />
  }
  return <PcdnImage src={engine.faviconUrl} className='engine-icon' />
}
