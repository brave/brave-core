/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useShieldsApi } from '../api/shields_api_context'
import { getString } from './strings'

import { style } from './adblock_only_notice.style'

export function AdBlockOnlyNotice() {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()

  if (!siteBlockInfo) {
    return null
  }

  const shieldsEnabled = siteBlockInfo.isBraveShieldsEnabled
  const adblockOnlyEnabled = siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled

  const showNotice = shieldsEnabled && adblockOnlyEnabled
  if (!showNotice) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <div>
        <div>{getString('BRAVE_SHIELDS_AD_BLOCK_ONLY_MODE_ENABLE_TITLE')}</div>
        <p>{getString('BRAVE_SHIELDS_AD_BLOCK_ONLY_MODE_ENABLE_DESC')}</p>
      </div>
      <button onClick={() => {}}>
        <Icon name='launch' />
      </button>
    </div>
  )
}
