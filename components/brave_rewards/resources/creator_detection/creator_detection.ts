// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { log, throttle } from './helpers'

// A parameter whose value is injected by the browser. Sets a callback that will
// be executed when the page loads or when the URL is updated.
declare function setPageChangedCallback(
  callback: () => any): void

// A parameter whose value is injected by the browser. Notifies the browser that
// a creator has been detected for the current URL.
declare function onCreatorDetected(
  id: string,
  name: string,
  url: string,
  imageURL: string): void

// A parameter whose value is injected by the browser. Indicates that scripts
// are allowed to log additional information for debugging purposes.
declare const verboseLogging: boolean

export interface CreatorInfo {
  id: string
  name: string
  url: string
  imageURL: string
}

type DetectionHandler = () => Promise<CreatorInfo | null>

export function initializeDetector(initializer: () => DetectionHandler) {
  if (verboseLogging) {
    log.verbose = true
  }

  const detectCreator = throttle(initializer())
  let currentURL = ''

  setPageChangedCallback(async () => {
    if (location.href === currentURL) {
      return
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
      onCreatorDetected(
        creator.id,
        creator.name,
        creator.url,
        creator.imageURL
      )
    } else {
      log.info('No creator found for current URL')
      onCreatorDetected('', '', '', '')
    }
  })
}


