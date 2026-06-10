/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { AdsHistoryItem, AdLikeStatus } from '../lib/app_store'
import * as mojom from './mojom'
import { adTypeFromMojom, convertMojoTime } from './mojom_helpers'

function likeStatusFromReactionType(
  reaction: mojom.ReactionType,
): AdLikeStatus {
  switch (reaction) {
    case mojom.ReactionType.kLiked:
      return 'liked'
    case mojom.ReactionType.kDisliked:
      return 'disliked'
    default:
      return ''
  }
}

function reactionTypeFromLikeStatus(status: AdLikeStatus): mojom.ReactionType {
  switch (status) {
    case 'liked':
      return mojom.ReactionType.kLiked
    case 'disliked':
      return mojom.ReactionType.kDisliked
    default:
      return mojom.ReactionType.kNeutral
  }
}

function convertItem(
  id: string,
  item: mojom.AdHistoryItemInfo,
): AdsHistoryItem | null {
  const type = adTypeFromMojom(item.type)
  if (!type) {
    return null
  }
  const url = item.targetUrl.url
  if (!item.title || !url) {
    console.error('Unexpected Ad history item', item)
    return null
  }
  return {
    id,
    type,
    createdAt: convertMojoTime(item.createdAt),
    name: item.title,
    text: item.description,
    domain: new URL(url).hostname,
    url,
    likeStatus: likeStatusFromReactionType(item.likeAdReaction),
    inappropriate: item.isFlagged,
  }
}

class AdsHistoryAdapter {
  private items_ = new Map<string, mojom.AdHistoryItemInfo>()

  setItems(items: mojom.AdHistoryItemInfo[]) {
    this.items_.clear()
    for (const item of items) {
      this.items_.set(this.items_.size.toString(), item)
    }
  }

  getReaction(id: string): mojom.ReactionInfo | null {
    const item = this.items_.get(id)
    if (!item) {
      return null
    }
    return {
      mojomAdType: item.type,
      creativeInstanceId: item.creativeInstanceId,
      creativeSetId: item.creativeSetId,
      campaignId: item.campaignId,
      advertiserId: item.advertiserId,
      segment: item.segment,
    }
  }

  setAdLikeStatus(id: string, status: AdLikeStatus): AdLikeStatus {
    const item = this.items_.get(id)
    if (!item) {
      return ''
    }
    const previous = likeStatusFromReactionType(item.likeAdReaction)
    item.likeAdReaction = reactionTypeFromLikeStatus(status)
    return previous
  }

  setInappropriate(id: string, inappropriate: boolean) {
    const item = this.items_.get(id)
    if (item) {
      item.isFlagged = inappropriate
    }
  }

  getItems(): AdsHistoryItem[] {
    const items: AdsHistoryItem[] = []
    for (const [id, item] of this.items_) {
      const converted = convertItem(id, item)
      if (converted) {
        items.push(converted)
      }
    }
    return items
  }
}

export function createAdsHistoryAdapter() {
  return new AdsHistoryAdapter()
}
