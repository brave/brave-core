/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { AdType } from '../lib/types'
import { LocaleContext } from '../../shared/lib/locale_context'
import { adsPerHourOptions } from '../../shared/lib/ads_options'
import { ToggleButton } from '../../shared/components/toggle_button'

import * as style from './ads_control_view.style'

export function AdsControlView () {
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()

  const { adsData } = useRewardsData((state) => ({
    adsData: state.adsData
  }))

  const adCount = (type: AdType, enabled: boolean) => {
    return (
      <span className={enabled ? '' : 'disabled'}>
        {adsData.adTypesReceivedThisMonth[type] || 0}
      </span>
    )
  }

  const onNewTabAdsEnabledChange = (enabled: boolean) => {
    actions.onAdsSettingSave('newTabAdsEnabled', enabled)
  }

  const onNotificationsEnabledChange = (enabled: boolean) => {
    actions.onAdsSettingSave('notificationAdsEnabled', enabled)
  }

  const onAdsPerHourChange = (event: React.FormEvent<HTMLSelectElement>) => {
    const value = Number(event.currentTarget.value) || 0
    actions.onAdsSettingSave('adsPerHour', value)
  }

  return (
    <style.root>
      <style.adTypeRow>
        <style.adTypeControls>
          <style.adTypeLabel>
            {getString('newTabAdCountLabel')}
          </style.adTypeLabel>
          <style.adTypeToggle>
            <ToggleButton
              checked={adsData.newTabAdsEnabled}
              onChange={onNewTabAdsEnabledChange}
            />
          </style.adTypeToggle>
          <style.adTypeConfig />
        </style.adTypeControls>
        <style.adTypeCount>
          {adCount('new_tab_page_ad', adsData.newTabAdsEnabled)}
        </style.adTypeCount>
      </style.adTypeRow>

      <style.adTypeRow>
        <style.adTypeControls>
          <style.adTypeLabel>
            {getString('notificationAdCountLabel')}
          </style.adTypeLabel>
          <style.adTypeToggle>
            <ToggleButton
              checked={adsData.notificationAdsEnabled}
              onChange={onNotificationsEnabledChange}
            />
          </style.adTypeToggle>
          <style.adTypeConfig>
            <select
              value={adsData.adsPerHour}
              disabled={!adsData.notificationAdsEnabled}
              onChange={onAdsPerHourChange}
            >
              {
                adsPerHourOptions
                  .filter((n) => n || n === adsData.adsPerHour)
                  .map((n) => (
                    <option key={n} value={n}>
                      {getString(`adsPerHour${n}`)}
                    </option>
                  ))
              }
            </select>
          </style.adTypeConfig>
        </style.adTypeControls>
        <style.adTypeCount>
          {adCount('ad_notification', adsData.notificationAdsEnabled)}
        </style.adTypeCount>
      </style.adTypeRow>

      <style.adTypeRow>
        <style.adTypeControls>
          <style.adTypeLabel>
            <div>{getString('newsAdCountLabel')}</div>
            <style.adTypeInfo>
              <Icon name='info-outline' />
              <div className='tooltip'>
                <style.infoTooltip>
                  {
                    adsData.newsAdsEnabled
                      ? getString('newsAdInfo')
                      : getString('newsAdInfoDisabled')
                  }
                </style.infoTooltip>
              </div>
            </style.adTypeInfo>
          </style.adTypeLabel>
          <style.adTypeToggle />
          <style.adTypeConfig />
        </style.adTypeControls>
        <style.adTypeCount>
          {adCount('inline_content_ad', adsData.newsAdsEnabled)}
        </style.adTypeCount>
      </style.adTypeRow>
    </style.root>
  )
}
