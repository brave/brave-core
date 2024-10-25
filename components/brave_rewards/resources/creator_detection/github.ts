// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CreatorInfo, initializeDetector } from './creator_detection'
import { log, getPathComponents } from './helpers'

initializeDetector(() => {

  const nonUserPaths = new Set([
    'about',
    'enterprise',
    'events',
    'explore',
    'home',
    'issues',
    'login',
    'logout',
    'marketplace',
    'nonprofit',
    'notifications',
    'orgs',
    'pricing',
    'pulls',
    'search',
    'settings',
    'team',
    'tos'
  ])

  function getScreenNameFromPath(path: string) {
    const parts = getPathComponents(path)
    if (parts.length === 0) {
      return ''
    }

    const screenName = parts[0]
    if (screenName === 'orgs' && parts.length > 1) {
      return parts[1]
    }

    if (nonUserPaths.has(screenName)) {
      return ''
    }

    return screenName
  }

  function getCreatorFromResponse(screenName: string, response: any): any {
    if (!response) {
      log.info('Empty response')
      return null
    }
    if (!response.id) {
      log.info('Missing "id" property')
      return null
    }
    return {
      id: `github#channel:${response.id}`,
      name: screenName,
      url: `https://github.com/${screenName}/`,
      imageURL: String(response.avatar_url || '')
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

    const hostname = 'api.' + location.hostname.split('.').slice(-2).join('.')
    const origin = location.origin.replace(location.hostname, hostname)
    const response = await fetch(`${origin}/users/${screenName}`)

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

  return async () => {
    const screenName = getScreenNameFromPath(location.pathname)
    if (!screenName) {
      log.info('No screen name for the current path')
      return null
    }
    return await fetchUserData(screenName)
  }

})
