// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export default class RealAdDemoCatalog {
  getAd (pageUrl: string, sizes: string[], isResponsive: boolean): Promise<chrome.braveRewards.PublisherAd[]> {
    return new Promise<chrome.braveRewards.PublisherAd[]>(resolve => {
      chrome.braveRewards.getPublisherAds(pageUrl, sizes, (ads: chrome.braveRewards.PublisherAd[]) => {
        if (!ads || !ads.length) {
          console.error('No ads came from braveRewards', ads)
          resolve([])
          return
        }
        resolve(ads)
      })
    })
  }
}
