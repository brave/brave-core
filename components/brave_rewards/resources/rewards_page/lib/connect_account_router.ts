/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { AppModelContext, useAppState } from './app_model_context'
import { RouterContext } from './router'

import * as routes from './app_routes'
import * as urls from '../../shared/lib/rewards_urls'

export function useConnectAccountRouter() {
  const model = React.useContext(AppModelContext)
  const router = React.useContext(RouterContext)
  const [isBubble] = useAppState((state) => [state.embedder.isBubble])

  return () => {
    if (isBubble) {
      model.openTab(urls.connectURL)
    } else {
      router.setRoute(routes.connectAccount)
    }
  }
}
