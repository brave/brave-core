/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

function dispatchRouteUpdated() {
  window.dispatchEvent(new Event('app-route-updated'))
}

interface RouteSource {
  get: () => string
  set: (route: string) => void
  replace: (route: string) => void
}

export class Router {
  _source: RouteSource

  constructor(source: RouteSource) {
    this._source = source
  }

  setRoute(route: string) {
    if (this._source.get() === route) {
      return
    }
    this._source.set(route)
    dispatchRouteUpdated()
  }

  replaceRoute(route: string) {
    if (this._source.get() === route) {
      return
    }
    this._source.replace(route)
    dispatchRouteUpdated()
  }

  getRoute() {
    return this._source.get()
  }
}

export const RouterContext = React.createContext<Router>(new Router({
  get: () => location.pathname.toLocaleLowerCase().replace(/\/$/, ''),
  set: (route) => history.pushState(null, '', route),
  replace: (route) => history.replaceState(null, '', route)
}))

// Returns the current page "route" (basically, a normalized pathname). The
// provided `onRouteUpdated` callback will be called when the route changes.
export function useRoute(
  onRouteUpdated?: (route: string, router: Router) => void,
  deps?: React.DependencyList
) {
  const router = React.useContext(RouterContext)
  const [route, setRoute] = React.useState(router.getRoute())

  React.useEffect(() => {
    const onRouteChange = () => setRoute(router.getRoute())

    window.addEventListener('popstate', onRouteChange)
    window.addEventListener('app-route-updated', onRouteChange)

    return () => {
      window.removeEventListener('popstate', onRouteChange)
      window.addEventListener('app-route-updated', onRouteChange)
    }
  }, [router])

  React.useEffect(() => {
    if (onRouteUpdated) {
      onRouteUpdated(route, router)
    }
  }, [route, router, onRouteUpdated, ...(deps ?? [])])

  return route
}

