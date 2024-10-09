// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CreatorInfo, initializeDetector } from './creator_detection'
import { log } from './helpers'

initializeDetector(() => {

  function getCreatorFromResponse(screenName: string, response: any): any {
    if (!response || response.kind !== 't2') {
      log.info('Unexpected "kind" value')
      return null
    }
    const { data } = response
    if (!data) {
      log.info('Missing "data" property')
      return null
    }
    if (!data.id || typeof data.id !== 'string') {
      log.info('Missing "id" property')
      return null
    }
    return {
      id: `reddit#channel:${data.id}`,
      name: screenName,
      url: `https://reddit.com/user/${screenName}/`,
      imageURL: String(data.icon_img || '')
    }
  }

  let currentCreator: CreatorInfo | null = null

  async function fetchUserData(screenName: string) {
    if (!screenName) {
      return null
    }

    if (currentCreator && currentCreator.name === screenName) {
      return currentCreator
    }

    log.info('Fetching profile data for', screenName)
    const response = await fetch(`/user/${screenName}/about.json`)
    if (!response.ok) {
      log.error('Unable to fetch profile data')
      return null
    }

    let responseJSON: any
    try {
      responseJSON = await response.json()
    } catch (err) {
      log.error('Error parsing profile data', err)
      return null
    }

    currentCreator = getCreatorFromResponse(screenName, responseJSON)
    if (!currentCreator) {
      log.info('Profile data response', responseJSON)
    }

    return currentCreator
  }

  function getScreenNameFromPath(path: string) {
    const match = path.match(/^\/user\/([^\/]+)/i)
    return match ? match[1] : ''
  }

  return async () => {
    const screenName = getScreenNameFromPath(location.pathname)
    if (!screenName) {
      log.info('No screen name for the current path')
      return null
    }
    return await fetchUserData(screenName)
  }

})
