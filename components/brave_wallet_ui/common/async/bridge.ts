// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../wallet_api_proxy'
import getWalletPanelApiProxy from '../../panel/wallet_panel_api_proxy'
import getWalletPageApiProxy from '../../page/wallet_page_api_proxy'

import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'
import { WalletApiDataOverrides } from '../../constants/testing_types'
import { unbiasedRandom } from '../../utils/random-utils'

// TODO: get from loadTimeData
const isIos = true

const debugProxyHandler = (path?: string) => ({
  get(target: any, propertyKey: PropertyKey, receiver?: unknown): any {
    const v = Reflect.get(target, propertyKey, receiver)

    const propertyPath = path
      ? `${path}.${propertyKey.toString()}`
      : propertyKey.toString()

    if (typeof v === 'function') {
      return (...args: any[]) => {
        console.log(`[debug] ${propertyPath} args=`, args)
        return v.apply(target, args)
      }
    }

    if (typeof v === 'object') {
      return new Proxy(v, debugProxyHandler(propertyPath))
    }

    return v
  }
})

const MIN_DELAY_MILLISECONDS = 16 // ~ 1 frame on a 60fps screen
const MAX_DELAY_MILLISECONDS = 33 // ~ 1 frame on a 30fps screen

const delayedProxyHandler = (path?: string) => ({
  get(target: any, propertyKey: PropertyKey, receiver?: unknown): any {
    const v = Reflect.get(target, propertyKey, receiver)

    const propertyPath = path
      ? `${path}.${propertyKey.toString()}`
      : propertyKey.toString()

    if (typeof v === 'function') {
      return (...args: any[]) => {
        const result = v.apply(target, args)
        if (result instanceof Promise) {
          return result.then(
            (result) =>
              new Promise((res) => {
                setTimeout(
                  () => {
                    res(result)
                  },
                  // wait time
                  unbiasedRandom(MIN_DELAY_MILLISECONDS, MAX_DELAY_MILLISECONDS)
                )
              })
          )
        }

        return result
      }
    }

    if (typeof v === 'object') {
      return new Proxy(v, delayedProxyHandler(propertyPath))
    }

    return v
  }
})

export function getAPIProxy(): WalletApiProxy {
  const nativeProxy =
    window.location.hostname === 'wallet-panel.top-chrome'
      ? getWalletPanelApiProxy()
      : getWalletPageApiProxy()

  const shouldDelay = isIos
  if (shouldDelay) {
    return new Proxy(nativeProxy, delayedProxyHandler())
  }

  const debug = window.localStorage.getItem(LOCAL_STORAGE_KEYS.DEBUG)
  if (!debug || debug !== 'true') {
    return nativeProxy
  }

  return new Proxy(nativeProxy, debugProxyHandler())
}

/** For testing */
export function resetAPIProxy(overrides?: WalletApiDataOverrides | undefined) {
  // no-op in production
}

export default getAPIProxy
