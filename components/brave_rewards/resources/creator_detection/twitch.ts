// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CreatorInfo, initializeDetector } from './creator_detection'
import { log, getPathComponents, pollFor } from './helpers'

initializeDetector(() => {

  function scrapeIcon() {
    const elem =
      document.querySelector('.channel-info-content .tw-avatar [src]')
    return (elem && elem.getAttribute('src')) || ''
  }

  function getChannelTitleElement() {
    return document.querySelector('h1.tw-title')
  }

  function scrapeChannelName() {
    const elem = getChannelTitleElement()
    return elem && elem.textContent || ''
  }

  function firstPathComponent(path: string) {
    return getPathComponents(path)[0] || ''
  }

  function isVideoPath() {
    return firstPathComponent(location.pathname) === 'videos'
  }

  const excludedPaths = new Set([
    'directory',
    'downloads',
    'jobs',
    'p',
    'search',
    'turbo'
  ])

  function scrapeChannelID() {
    if (isVideoPath()) {
      const elem = getChannelTitleElement()
      if (!elem || !elem.parentElement) {
        log.info('Unable to find parent of channel title element')
        return ''
      }
      const href = elem.parentElement.getAttribute('href')
      if (!href) {
        log.info('Missing "href" attribute in title element')
        return ''
      }
      let id = firstPathComponent(href)
      if (!id) {
        log.info('Unable to find channel ID in channel title "href":', href)
        return ''
      }
      log.info('Found channel ID in channel title element:', id)
      return id
    }

    const pathPart = firstPathComponent(location.pathname)
    if (!pathPart || excludedPaths.has(pathPart)) {
      log.info('Path does not support creator detection')
      return ''
    }

    log.info('Found channel ID in path:', pathPart)
    return pathPart
  }

  function scrapeUser() {
    const id = scrapeChannelID()
    if (!id) {
      return null
    }
    return {
      id: `twitch#author:${id}`,
      name: scrapeChannelName(),
      url: `${location.origin}/${id}`,
      imageURL: scrapeIcon()
    }
  }

  let currentUser: CreatorInfo | null = null

  function isPageLoadComplete() {
    const user = scrapeUser()
    if (!user) {
      return !isVideoPath()
    }
    if (!user.name || !user.imageURL) {
      return false
    }
    if (!currentUser) {
      return true
    }
    if (user.id === currentUser.id) {
      return true
    }
    return (
      user.name !== currentUser.name &&
      user.imageURL !== currentUser.imageURL
    )
  }

  function getUser() {
    if (!isPageLoadComplete()) {
      return null
    }
    currentUser = scrapeUser()
    return currentUser
  }

  return async () => {
    return await pollFor(getUser, {
      name: 'creator',
      interval: 500,
      timeout: 6000
    })
  }

})
