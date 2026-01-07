/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { WelcomeApi, createWelcomeApi } from './welcome_api'

const WelcomeApiContext = React.createContext<WelcomeApi | null>(null)

export function useWelcomeApi(): WelcomeApi {
  const api = React.useContext(WelcomeApiContext)
  if (!api) {
    throw new Error('useWelcomeApi must be used within WelcomeApiProvider')
  }
  return api
}

interface Props {
  children: React.ReactNode
  api?: WelcomeApi
}

export function WelcomeApiProvider(props: Props) {
  const api = props.api ?? createWelcomeApi()
  React.useEffect(() => () => api.close(), [api])
  return (
    <WelcomeApiContext.Provider value={api}>
      {props.children}
    </WelcomeApiContext.Provider>
  )
}
