/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { formatString } from '$web-common/formatString'
import { useAppState, useAppActions } from './app_context'
import { getString } from '../lib/strings'
import { MainToggle } from './main_toggle'

import { style } from './main_card.style'

export function MainCard() {
  const actions = useAppActions()
  const favicon = useAppState((s) => s.siteBlockInfo?.faviconUrl.url ?? '')
  const siteHost = useAppState((s) => s.siteBlockInfo?.host ?? '')
  const shieldsEnabled = useAppState((s) => {
    return s.siteBlockInfo?.isBraveShieldsEnabled ?? false
  })
  const trackersAndAdsBlocked = useAppState((s) => {
    return s.siteBlockInfo?.totalBlockedResources ?? 0
  })

  function renderBlockInfo() {
    if (!shieldsEnabled) {
      return (
        <div className='report-prompt'>
          {getString('BRAVE_SHIELDS_SITE_NOT_WORKING')}
          <Button onClick={actions.openWebCompatReporter}>
            {getString('BRAVE_SHIELDS_REPORT')}
          </Button>
        </div>
      )
    }
    if (trackersAndAdsBlocked <= 0) {
      return null
    }
    return (
      <div className='block-info'>
        <div className='items'></div>
        <div className='count'>{trackersAndAdsBlocked}</div>
        {getString('BRAVE_SHIELDS_TRACKERS_ADS_BLOCKED')}
      </div>
    )
  }

  return (
    <div
      data-css-scope={style.scope}
      data-shields-off={!shieldsEnabled}
    >
      <div className='site-info'>
        <div className='site-icon'>
          <img src={favicon} />
        </div>
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
        <MainToggle
          active={shieldsEnabled}
          disabled={false}
          onToggle={actions.setShieldsEnabled}
        />
      </div>
      {renderBlockInfo()}
    </div>
  )
}
