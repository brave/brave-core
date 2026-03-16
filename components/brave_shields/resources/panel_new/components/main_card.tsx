/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { formatString } from '$web-common/formatString'
import { useShieldsApi } from '../api/shields_api_context'
import { useResourceFaviconUrls } from './use_resource_favicon_urls'
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
          <Button onClick={api.openWebCompatWindow}>
            {getString('BRAVE_SHIELDS_REPORT')}
          </Button>
        </div>
      )
    }
    return (
      <div className='block-info'>
        <div className='blocked-items'>
          <BlockedFavicons />
        </div>
        {formatString(getString('BRAVE_SHIELDS_TRACKERS_ADS_BLOCKED'), {
          $1: () => <div className='count'>{trackersAndAdsBlocked}</div>,
        })}
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
          {favicon && (
            <img
              src={favicon}
              alt={siteHost}
            />
          )}
        </div>
        <div className='site-text'>
          <h3 className='overflow-ellipsis-start'>{siteHost}</h3>
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

function BlockedFavicons() {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const adsList = siteBlockInfo?.adsList ?? []
  const urls = adsList.map((entry) => entry.url)

  const faviconUrls = useResourceFaviconUrls(urls, {
    maxQueries: 16,
    maxResults: 3,
  })

  return (
    <>
      {faviconUrls.map((url) => (
        <div
          className='blocked-favicon'
          key={url}
        >
          <img
            className='favicon'
            alt={url}
            src={url}
          />
          <Icon name='disable-outline' />
        </div>
      ))}
    </>
  )
}
