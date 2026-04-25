/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { ShieldsApi } from './shields_api'

const ShieldsApiContext = React.createContext<ShieldsApi | null>(null)

export function useShieldsApi(): ShieldsApi {
  const api = React.useContext(ShieldsApiContext)
  if (!api) {
    throw new Error('useShieldsApi must be used within ShieldsApiProvider')
  }
  return api
}

interface ProviderProps {
  children: React.ReactNode
  api: ShieldsApi
}

export function ShieldsApiProvider({ children, api }: ProviderProps) {
  React.useEffect(() => () => api.close(), [api])
  return (
    <ShieldsApiContext.Provider value={api}>
      {children}
    </ShieldsApiContext.Provider>
  )
}
