// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { initializeDetector } from './creator_detection'
import { log, pollFor } from './helpers'

initializeDetector(() => {

  interface StateStore {
    getState: () => any
  }

  function getElementStore(elem: object | null) {
    if (!elem) {
      return null
    }
    for (const name of Object.getOwnPropertyNames(elem)) {
      if (name.startsWith('__reactProps$')) {
        let store: any = null
        try { store = (elem as any)[name].children.props.store }
        catch {}
        if (store && typeof store.getState === 'function') {
          return store as StateStore
        }
      }
    }
    return null
  }

  function findStore(elem: Element | null, depth = 0): StateStore | null {
    if (!elem) {
      return null
    }
    let store = getElementStore(elem)
    if (store) {
      return store
    }
    if (depth === 4) {
      return null
    }
    for (let child of elem.children) {
      store = findStore(child, depth + 1)
      if (store) {
        return store
      }
    }
    return null
  }

  let stateStore: StateStore | null = null

  function getStore() {
    if (!stateStore) {
      stateStore = findStore(document.getElementById('react-root'))
      if (stateStore) {
        log.info('State store found', stateStore)
      } else {
        log.info('Unable to find state store')
      }
    }
    return stateStore
  }

  function screenNamesMatch(name1: any, name2: any) {
    if (typeof name1 === 'string' && typeof name2 === 'string') {
      return name1.toLocaleLowerCase() === name2.toLocaleLowerCase()
    }
    return false
  }

  function getUserFromState(state: any, screenName: string) {
    const userEntities = state.entities.users.entities
    for (let [key, value] of Object.entries(userEntities) as any) {
      if (screenNamesMatch(value.screen_name, screenName)) {
        return {
          id: `twitter#channel:${key}`,
          name: screenName,
          url: `${location.origin}/${screenName}`,
          imageURL: String(value.profile_image_url_https || '')
        }
      }
    }
    log.info('Screen name not found in state', userEntities)
    return null
  }

  function getUserByScreenName(screenName: string) {
    const store = getStore()
    if (!store) {
      log.info('State store could not be found')
      return null
    }
    try {
      return getUserFromState(store.getState(), screenName)
    } catch (e) {
      log.error('Error attempting to get user state', e)
    }
    return null
  }

  function getScreenNameFromPath(path: string) {
    let match = path.match(/^\/([^\/]+)(\/|\/status\/[\s\S]+)?$/)
    if (match) {
      log.info('Found screen name in path:', match[1])
      return match[1]
    }
    log.info('Screen name not found in path')
    return null
  }

  function getUserFromPath(path: string) {
    const screenName = getScreenNameFromPath(path)
    if (screenName) {
      const user = getUserByScreenName(screenName)
      if (user) {
        return user
      }
    }
    return null
  }

  return async () => {
    return await pollFor(() => getUserFromPath(location.pathname), {
      name: 'creator',
      interval: 1000,
      timeout: 6000
    })
  }

})
