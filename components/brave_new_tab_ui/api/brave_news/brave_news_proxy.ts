// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'gen/mojo/public/js/mojo_bindings_lite.js'
import 'gen/mojo/public/mojom/base/time.mojom-lite.js'
import 'gen/url/mojom/url.mojom-lite.js'
import 'gen/brave/components/brave_today/common/brave_news.mojom-lite.js'

export type PaddedImage = {
  paddedImageUrl: url.mojom.Url
}

export type UnpaddedImage = {
  imageUrl: url.mojom.Url
}

type Image = PaddedImage | UnpaddedImage


declare namespace url.mojom {
  export type Url = {
    url: string
  }
}

export type DisplayAd = {
  uuid: string
  creativeInstanceId: string
  dimensions: string
  title: string
  description: string
  image: Image
  targetUrl: url.mojom.Url
  ctaText?: string
}

export type BraveNewsController = {
  getFeed: () => Promise<any>
  getPublishers: () => Promise<any>
  getImageData: (padded_image_url: url.mojom.Url) => Promise<{ imageData: Uint8Array | undefined }>
  setPublisherPref: (publisherId: string, newStatus: braveNews.mojom.UserEnabled) => void
  clearPrefs(): () => void
  isFeedUpdateAvailable: (displayed_feed_hash: string) => Promise<{ isUpdateAvailable: boolean }>
  getDisplayAd:() => Promise<{ ad: DisplayAd | undefined }>
  onInteractionSessionStarted: () => void
  onSessionCardVisitsCountChanged: (cardsVisitedSessionTotalCount: number) => void
  onSessionCardViewsCountChanged: (cardsViewedSessionTotalCount: number) => void
  onPromotedItemView: (itemId: string, creativeInstanceId: string) => void
  onPromotedItemVisit: (itemId: string, creativeInstanceId: string) => void
  onDisplayAdVisit: (itemId: string, creativeInstanceId: string) => void
  onDisplayAdView: (itemId: string, creativeInstanceId: string) => void
}

const braveNewsControllerInstance =
    braveNews.mojom.BraveNewsController.getRemote() as unknown as BraveNewsController;

export default braveNewsControllerInstance
