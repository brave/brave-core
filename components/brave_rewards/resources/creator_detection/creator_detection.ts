// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { log, throttle } from './helpers'

export interface CreatorInfo {
  id: string
  name: string
  url: string
  imageURL: string
}

type DetectionHandler = () => Promise<CreatorInfo | null>

function setPageDetectionHandler(detectCreator: DetectionHandler) {
  let { braveRewards } = self as any
  if (!braveRewards) {
    braveRewards = {}
    Object.assign(self, { braveRewards })
  }
  Object.assign(braveRewards, { detectCreator })
}

export function initializeDetector(initializer: () => DetectionHandler) {
  const detectCreator = throttle(initializer())
  let currentURL = ''

  setPageDetectionHandler(async () => {
    if (location.href === currentURL) {
      return null
    }

    currentURL = location.href

    let creator: CreatorInfo | null = null
    try {
      creator = await detectCreator()
    } catch (err) {
      log.error('Error detecting creator', err)
    }

    if (creator) {
      log.info('Found creator:', creator)
    } else {
      log.info('No creator found for current URL')
    }

    return creator
  })
}


