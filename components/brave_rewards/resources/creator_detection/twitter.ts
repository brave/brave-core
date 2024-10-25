// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CreatorInfo, initializeDetector } from './creator_detection'
import { log, pollFor } from './helpers'

function getUserFromAppState(eventName: string, screenName: string) {
  interface StateStore {
    getState: () => any
  }

  let messages: any[] = []

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
    messages = ['Screen name not found in state']
    return null
  }

  function getUserByScreenName() {
    const store = findStore(document.getElementById('react-root'))
    if (!store) {
      messages = ['State store could not be found']
      return null
    }
    const state = store.getState()
    try {
      return getUserFromState(state, screenName)
    } catch (e) {
      console.error(e)
      messages = ['Error attempting to get user state', state]
    }
    return null
  }

  document.dispatchEvent(new CustomEvent(eventName, {
    detail: { user: getUserByScreenName(), messages }
  }))
}

initializeDetector(() => {
  async function getUserFromState(screenName: string) {
    return new Promise<CreatorInfo | null>((resolve) => {
      const eventName = 'brave-rewards-user-data'
      const listener = (event: CustomEvent) => {
        document.removeEventListener(eventName, listener)
        const { user, messages } = event.detail || {}
        resolve(user || null)
        if (Array.isArray(messages) && messages.length > 0) {
          log.info(...messages)
        }
      }
      document.addEventListener(eventName, listener)
      const script = document.createElement('script')
      script.textContent = `(${getUserFromAppState})(
        ${JSON.stringify(eventName)},
        ${JSON.stringify(screenName)})`
      document.head.appendChild(script)
      document.head.removeChild(script)
    })
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

  async function getUserFromPath(path: string) {
    const screenName = getScreenNameFromPath(path)
    if (screenName) {
      const user = await getUserFromState(screenName)
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
