/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { AdsHistoryItem, AdLikeStatus } from '../lib/app_state'

// Currently, the Ads service provides Ads history data as untyped `base::Value`
// (JSON) data. In order to interact with these history items, the caller must
// provide the original JSON data for the item with the Ads service method call.
// This module provides an adapter that coverts the raw JSON data into front-end
// TS interfaces, and allows retrieval of the raw JSON data when an update is
// required.

interface MapValue {
  createdAt: number
  detail: any
}

function createMap(data: any[]) {
  const map = new Map<string, MapValue>()
  for (const group of data) {
    const createdAt = Number(group.timestampInMilliseconds) || 0
    for (const detail of group.adDetailRows) {
      if (detail && typeof detail === 'object') {
        map.set(map.size.toString(), { createdAt, detail })
      }
    }
  }
  return map
}

function convertLikeAction(likeAction: number) {
  switch (likeAction) {
    case 1: return 'liked'
    case 2: return 'disliked'
    default: return ''
  }
}

function convertMapValue(id: string, value: MapValue): AdsHistoryItem | null {
  const { detail } = value
  const { adContent } = detail
  const item: AdsHistoryItem = {
    id,
    createdAt: value.createdAt,
    name: String(adContent.brand || ''),
    text: String(adContent.brandInfo || ''),
    domain: String(adContent.brandDisplayUrl || ''),
    url: String(adContent.brandUrl || ''),
    likeStatus: convertLikeAction(adContent.likeAction),
    inappropriate: Boolean(adContent.flaggedAd)
  }
  if (!item.name || !item.domain || !item.url) {
    console.error('Unexpected Ad history data', detail)
    return null
  }
  return item
}

class AdsHistoryAdapter {
  map_ = new Map<string, MapValue>()

  parseData(data: string) {
    this.map_ = createMap(JSON.parse(data))
  }

  getRawDetail(id: string) {
    return JSON.stringify(this.map_.get(id)?.detail || '{}')
  }

  getItem(id: string) {
    const value = this.map_.get(id)
    if (!value) {
      return null
    }
    return convertMapValue(id, value)
  }

  setAdLikeStatus(id: string, status: AdLikeStatus): AdLikeStatus {
    const value = this.map_.get(id)
    if (!value) {
      return ''
    }
    let action = 0
    switch (status) {
      case 'liked':
        action = 1
        break
      case 'disliked':
        action = 2
        break
    }
    const { adContent } = value.detail
    const previous = convertLikeAction(adContent.likeAction)
    adContent.likeAction = action
    return previous
  }

  setInappropriate(id: string, inappropriate: boolean) {
    const value = this.map_.get(id)
    if (value) {
      const { adContent } = value.detail
      adContent.flaggedAd = inappropriate
    }
  }

  getItems() {
    const items: AdsHistoryItem[] = []
    for (const [key, value] of this.map_.entries()) {
      const item = convertMapValue(key, value)
      if (item) {
        items.push(item)
      }
    }
    return items
  }
}

export function createAdsHistoryAdapter() {
  return new AdsHistoryAdapter()
}
