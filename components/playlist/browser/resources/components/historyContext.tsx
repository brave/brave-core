// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import { History } from 'history'

export let history: History | undefined

export function HistoryContext (props: React.PropsWithChildren<{}>) {
  const h = useHistory()
  const location = useLocation()

  React.useEffect(() => {
    history = h
    return () => (history = undefined)
  }, [h])

  // When the component mounts, restore the location from local storage.
  // This happens only when the initial location is the default location.
  React.useEffect(() => {
    const lastLocation = localStorage.getItem('lastLocation')
    if (location.pathname === '/' && lastLocation) {
      history?.replace(lastLocation)
    }
  }, [])

  // Record the current location in local storage
  React.useEffect(() => {
    localStorage.setItem('lastLocation', location.pathname)
  }, [location])

  return <>{props.children}</>
}
