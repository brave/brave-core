/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Toggle from '@brave/leo/react/toggle'

import { formatString } from '$web-common/formatString'
import { useShieldsApi } from '../api/shields_api_context'
import { getString } from './strings'

import { style } from './main_card.style'

export function MainCard() {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()

  if (!siteBlockInfo) {
    return null
  }

  const favicon = siteBlockInfo.faviconUrl.url
  const siteHost = siteBlockInfo.host
  const shieldsEnabled = siteBlockInfo.isBraveShieldsEnabled
  const trackersAndAdsBlocked = siteBlockInfo.totalBlockedResources

  function renderBlockInfo() {
    if (!shieldsEnabled) {
      return (
        <div className='report-prompt'>
          {getString('BRAVE_SHIELDS_SITE_NOT_WORKING')}
          <Button onClick={api.actions.openWebCompatWindow}>
            {getString('BRAVE_SHIELDS_REPORT')}
          </Button>
        </div>
      )
    }
    return (
      <div className='block-info'>
        <div className='items'></div>
        <div className='count'>{trackersAndAdsBlocked}</div>
        <span>{getString('BRAVE_SHIELDS_TRACKERS_ADS_BLOCKED')}</span>
      </div>
    )
  }

  return (
    <div
      data-css-scope={style.scope}
      data-shields-off={!shieldsEnabled}
    >
      <div className='site-info'>
        <div className='site-icon'>{favicon && <img src={favicon} />}</div>
        <div className='site-text'>
          <h3>{siteHost}</h3>
          <div className='shields-status'>
            {formatString(
              shieldsEnabled
                ? getString('BRAVE_SHIELDS_STATUS_UP')
                : getString('BRAVE_SHIELDS_STATUS_DOWN'),
              {
                $1: (content) => <strong>{content}</strong>,
              },
            )}
          </div>
        </div>
        <Toggle
          checked={shieldsEnabled}
          onChange={(event) => api.setBraveShieldsEnabled([event.checked])}
        />
      </div>
      {renderBlockInfo()}
    </div>
  )
}
