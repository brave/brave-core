// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { initializeDetector } from './creator_detection'
import { log, pollFor, urlPath, absoluteURL } from './helpers'

initializeDetector(() => {

  function getChannelFromURL(url: string) {
    let pathname = urlPath(url)
    let match = pathname.match(/^\/channel\/([^\/]+)/i)
    if (match) {
      return match[1]
    }
    return ''
  }

  function getVideoIdFromURL(url: string) {
    if (!urlPath(url).match(/^\/watch(\/|$)/i)) {
      return ''
    }
    let params = null
    try {
      params = new URL(url).searchParams
    } catch {
      return ''
    }
    return params.get('v') || ''
  }

  function getChannelNameFromURL(url: string) {
    let pathname = urlPath(url)
    let match = pathname.match(/^\/@([^\/]+)/)
    if (match) {
      return match[1]
    }
    return ''
  }

  type PathType = 'channel' | 'channel-name' | 'video' | ''

  function getPathType(): PathType {
    if (getChannelFromURL(location.href)) {
      return 'channel'
    }
    if (getChannelNameFromURL(location.href)) {
      return 'channel-name'
    }
    if (getVideoIdFromURL(location.href)) {
      return 'video'
    }
    return ''
  }

  let firstChannelScrape = true
  let currentPathType: PathType = ''

  // Attempts to find the channel identifier by inspecting the URL and various
  // DOM elements.
  function scrapeChannel() {
    let firstScrape = firstChannelScrape
    firstChannelScrape = false

    let channel = getChannelFromURL(location.href)
    if (channel) {
      return channel
    }

    let elem = document.querySelector<HTMLAnchorElement>('a[aria-label=About]')
    if (elem) {
      channel = getChannelFromURL(elem.href)
      if (channel) {
        return channel
      }
    }

    elem = document.querySelector<HTMLAnchorElement>('a.ytp-ce-channel-title')
    if (elem) {
      channel = getChannelFromURL(elem.href)
      if (channel) {
        return channel
      }
    }

    // On initial page load, a "canonical" link element that contains the
    // channel identifier may be present on channel pages. On subsequent
    // same-document navigations, this element will still exist but will no
    // longer be valid. Only look for this element the first time we attempt to
    // find the channel ID on the page (i.e. the initial page load).
    if (firstScrape) {
      let elem = document.querySelector<HTMLLinkElement>('link[rel=canonical]')
      if (elem) {
        channel = getChannelFromURL(elem.href)
        if (channel) {
          return channel
        }
      }
    }

    return ''
  }

  function scrapeChannelName() {
    let name = getChannelNameFromURL(location.href)
    if (name) {
      return name
    }

    let elems = document.querySelectorAll<HTMLAnchorElement>([
      'ytd-video-owner-renderer a',
      'ytm-slim-owner-renderer a'
    ].join(','))

    for (let elem of elems) {
      name = getChannelNameFromURL(elem.href)
      if (name) {
        return name
      }
    }

    if (currentPathType === 'channel' || currentPathType === 'channel-name') {
      let header = document.querySelector<HTMLElement>('#page-header')
      if (header) {
        let matches = header.innerText.match(/@[\w]+/)
        if (matches && matches.length > 0) {
          return matches[0]
        }
      }
    }

    return ''
  }

  function scrapeImage() {
    let elems = document.querySelectorAll<HTMLImageElement>([
      '#avatar img',
      'yt-page-header-renderer yt-avatar-shape img',
      'ytm-slim-owner-renderer a img'
    ].join(','))

    for (let elem of elems) {
      if (elem.src) {
        return elem.src
      }
    }

    return ''
  }

  function scrapeChannelURL() {
    let name = scrapeChannelName()
    if (name) {
      return `${location.origin}/@${name}`
    }
    if (currentPathType === 'channel') {
      return location.href
    }
    return ''
  }

  async function getChannel() {
    let channel = scrapeChannel()
    if (channel) {
      return channel
    }
    let name = scrapeChannelName()
    if (name) {
      let channelURL = `/@${name}`
      log.info(`Fetching "${channelURL}" for channel ID detection`)
      let response = await fetch(channelURL)
      let responseText = response.ok ? await response.text() : ''
      let match = responseText.match(/<link itemprop="url" href="([^"]+)"/i)
      if (match) {
        log.info('Found channel ID:', match[1])
        return getChannelFromURL(match[1])
      } else {
        log.info('Unable to find channel ID in channel URL')
      }
    } else {
      log.info('Unable to determine channel ID')
    }
    return ''
  }

  async function getUser() {
    if (!currentPathType) {
      return null
    }
    let channel = await getChannel()
    if (channel) {
      return {
        id: `youtube#channel:${channel}`,
        name: scrapeChannelName(),
        url: scrapeChannelURL(),
        imageURL: scrapeImage()
      }
    }
    return null
  }

  function isPageLoadComplete() {
    if (currentPathType === 'channel') {
      let elem = document.querySelector<HTMLAnchorElement>(
        'a.ytp-ce-channel-title'
      )
      if (elem) {
        return absoluteURL(elem.href) === location.href
      }
      return Boolean(document.querySelector('#page-header'))
    }

    if (currentPathType === 'channel-name') {
      let elem = document.querySelector<HTMLAnchorElement>(
        'ytd-video-owner-renderer a'
      )
      if (elem && absoluteURL(elem.href) === location.href) {
        return true
      }
      let form = document.querySelector<HTMLFormElement>('#form')
      let name = getChannelNameFromURL(location.href)
      return Boolean(form && urlPath(form.action).startsWith(`/@${name}`))
    }

    if (currentPathType === 'video') {
      let id = getVideoIdFromURL(location.href)
      let elem = document.querySelector('ytd-watch-metadata')
      return Boolean(elem && elem.getAttribute('video-id') === id)
    }

    return true
  }

  return async () => {
    currentPathType = getPathType()
    log.info(`Current path type: ${currentPathType} (${location.pathname})`)

    await pollFor(isPageLoadComplete, {
      name: 'load complete',
      interval: 500,
      timeout: 6000
    })

    return await getUser()
  }

})
