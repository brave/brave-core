// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect, useState } from "react"

// This file provides a helper for extract route params from a path and
// notifying consumers when those path params change.

// Extracts routeParams from a string:
// /foo/{bar}/baz ==> { bar: string }
// /foo/{bar}/{baz} ==> { bar: string }
// /foo/{bar}/{baz} ==> { bar: string, baz: string }
type RouteParamsAssumeTrailing<T extends string> = T extends `${infer Segment}/${infer Rest}`
  // Check if the current segment is a param
  ? (Segment extends `{${infer ParamName}}`
    ? {
      [P in ParamName]: string
    }
    : {}) & RouteParamsAssumeTrailing<Rest>
  // Basecase is no params
  : {}

// Ensures there's a trailing slash in the tokens to be consumed
type RouteParams<T extends string> = T extends `${string}/`
  ? RouteParamsAssumeTrailing<T>
  : RouteParamsAssumeTrailing<`${T}/`>

// Parses a param name from a path segment (or null, if the segment is not a
// param)
// {foo} ==> foo
// bar ==> null
const getParamName = (part: string) => {
  if (part.startsWith("{") && part.endsWith("}")) {
    return part.substring(1, part.length - 1)
  }
  return null
}

// Finds a matching route for a given URL - if found returns a tuple of
// 1. The route (for looking up in the routes map)
// 2. The list of callbacks for the given route
// 3. The parsed parameters for the route.
const extractParams = (currentUrl: string, route: string) => {
  const path = new URL(currentUrl).pathname
  const pathParts = path.split('/')
  const routeParts = route.split('/')

  if (routeParts.length !== pathParts.length) return null

  const params: { [key: string]: string } = {}
  for (let i = 0; i < pathParts.length; ++i) {
    const routePart = routeParts[i]
    const pathPart = pathParts[i]

    const paramName = getParamName(routePart)
    // If its a param, store the value
    if (paramName) {
      params[paramName] = pathPart
    } else if (routePart !== pathPart) {
      // The path doesn't match, so bail
      return null
    }
  }

  return params
}

// Extracts and returns matching route params from a URL, or null if the pattern
// doesn't match.
// Additionally, when the route matches navigations will be intercepted,
// preventing a page reload.
export const useRoute = <Route extends string>(route: Route): RouteParams<Route> | null => {
  const [params, setParams] = useState(() => extractParams(window.location.href, route))

  useEffect(() => {
    const handler = (e: NavigateEvent) => {
      const params = extractParams(e.destination.url, route)
      if (params) e.intercept()
      setParams(params)
    }
    window.navigation.addEventListener('navigate', handler)
    return () => {
      window.navigation.removeEventListener('navigate', handler)
    }
  }, [route])

  return params as RouteParams<Route> | null
}
