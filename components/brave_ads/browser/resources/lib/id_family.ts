/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState } from './app_context'
import { Campaign } from './app_store'

// The Advertiser ID/Campaign ID/Creative Set ID/Creative Instance ID that a
// given id belongs to; only the Campaigns tab's data ever has all four
// related in one place. Fields above the id that was looked up are always
// present; fields below it are omitted, since a broader id (e.g. a Campaign
// ID) maps to more than one narrower one (e.g. many Creative Set IDs).
export interface IdFamily {
  advertiserId: string
  campaignId: string
  creativeSetId?: string
  creativeInstanceId?: string
}

function buildIdFamilyMap(campaigns: Campaign[]) {
  const map = new Map<string, IdFamily>()
  for (const campaign of campaigns) {
    const advertiserId = campaign['Advertiser ID']
    const campaignId = campaign['Campaign ID']
    const campaignFamily: IdFamily = { advertiserId, campaignId }
    if (!map.has(advertiserId)) {
      map.set(advertiserId, campaignFamily)
    }
    if (!map.has(campaignId)) {
      map.set(campaignId, campaignFamily)
    }

    for (const creativeSet of campaign['Creative Sets']) {
      const creativeSetId = creativeSet['Creative Set ID']
      const creativeSetFamily: IdFamily = {
        advertiserId,
        campaignId,
        creativeSetId,
      }
      if (!map.has(creativeSetId)) {
        map.set(creativeSetId, creativeSetFamily)
      }

      for (const creative of creativeSet['Creatives']) {
        const creativeInstanceId = creative['Creative Instance ID']
        map.set(creativeInstanceId, {
          ...creativeSetFamily,
          creativeInstanceId,
        })
      }
    }
  }
  return map
}

export function useIdFamily(id: string): IdFamily | undefined {
  const notificationAdCampaigns =
    useAppState((state) => state.activeNotificationAdCampaigns)
  const newTabPageAdCampaigns =
    useAppState((state) => state.activeNewTabPageAdCampaigns)

  const map = React.useMemo(
    () => buildIdFamilyMap([...notificationAdCampaigns, ...newTabPageAdCampaigns]),
    [notificationAdCampaigns, newTabPageAdCampaigns],
  )

  return map.get(id)
}

export function formatIdFamily(family: IdFamily): string {
  const lines = [
    `Advertiser ID: ${family.advertiserId}`,
    `Campaign ID: ${family.campaignId}`,
  ]
  if (family.creativeSetId !== undefined) {
    lines.push(`Creative Set ID: ${family.creativeSetId}`)
  }
  if (family.creativeInstanceId !== undefined) {
    lines.push(`Creative Instance ID: ${family.creativeInstanceId}`)
  }
  return lines.join('\n')
}
