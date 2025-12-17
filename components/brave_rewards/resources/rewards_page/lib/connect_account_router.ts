/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useAppState, useAppActions } from './app_context'
import { RouterContext } from './router'

import * as routes from './app_routes'
import * as urls from '../../shared/lib/rewards_urls'

export function useConnectAccountRouter() {
  const actions = useAppActions()
  const router = React.useContext(RouterContext)
  const isBubble = useAppState((state) => state.embedder.isBubble)

  return () => {
    if (isBubble) {
      actions.openTab(urls.connectURL)
    } else {
      router.setRoute(routes.connectAccount)
    }
  }
}

export function useSwitchAccountRouter() {
  const actions = useAppActions()
  const router = React.useContext(RouterContext)
  const isBubble = useAppState((state) => state.embedder.isBubble)

  return () => {
    if (isBubble) {
      actions.openTab(urls.switchAccountURL)
    } else {
      router.setRoute(routes.switchAccount)
    }
  }
}
