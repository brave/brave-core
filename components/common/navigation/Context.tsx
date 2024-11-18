// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect } from "react";
import * as React from "react";

// Dictionary of url params, parsed from current location
export type NavigationParams = { [key: string]: string | undefined }

// Map of route to param handlers
export type Routes = Map<string, Array<undefined | ((params: NavigationParams) => void)>>;

export interface NavigationContext {
    params: NavigationParams
    addRoute: (route: string, callback?: (params: any) => void) => void
    removeRoute: (route: string, callback?: (params: any) => void) => void
}

const Context = React.createContext<NavigationContext>({
    params: {},
    addRoute: () => { },
    removeRoute: () => { }
})

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
const findMatchingRoute = (url: string, routes: Routes) => {
    const path = new URL(url).pathname
    const pathParts = path.split('/')


    for (const [route, callbacks] of routes.entries()) {
        const routeParts = route.split('/')

        if (routeParts.length !== pathParts.length) continue

        let isMatch = true
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
                isMatch = false
                break
            }
        }

        if (isMatch) {
            return [route, callbacks, params] as const
        }
    }

    // Couldn't find a handler
    return [null, [], {}] as const
}

// Accesses the current NavigationParams
export function useParams<T extends NavigationParams = NavigationParams>() {
    return React.useContext(Context).params as T
}

export function useNavigation<T extends NavigationParams = NavigationParams>() {
    return React.useContext(Context) as NavigationContext & { params: T }
}

// Listens to Navigations and provides up to date navigation params to consumers.
export function NavigationContext(props: React.PropsWithChildren) {
    const routes = React.useRef<Routes>(new Map())
    const [params, setParams] = React.useState<NavigationParams>({})

    useEffect(() => {
        const handler = (e: NavigateEvent) => {
            const [, callbacks, params] = findMatchingRoute(e.destination.url, routes.current)

            if (!handler) {
                setParams({})
                return
            }

            setParams(params)
            e.intercept({
                handler: async () => {
                    for (const callback of callbacks) {
                        callback?.(params)
                    }
                }
            })
        }

        window.navigation.addEventListener('navigate', handler)
        return () => {
            window.navigation.removeEventListener('navigate', handler)
        }
    }, [])

    const addRoute = React.useCallback((route: string, callback?: (params: NavigationParams) => void) => {
        if (!routes.current.has(route)) {
            routes.current.set(route, [])
        }

        routes.current.get(route)!.push(callback)
        const [matchingRoute, , params] = findMatchingRoute(location.href, routes.current)
        if (matchingRoute === route) {
            setParams(params)
            callback?.(params)
        }
    }, [])

    const removeRoute = React.useCallback((route: string, callback?: (params: NavigationParams) => void) => {
        const callbacks = routes.current.get(route) ?? []
        const index = callbacks.indexOf(callback)
        if (index !== -1) {
            callbacks.splice(index, 1)
        }

        if (callbacks.length === 0) {
            routes.current.delete(route)
        }

        setParams(findMatchingRoute(location.href, routes.current)[2])
    }, [])

    return <Context.Provider value={{ params, addRoute, removeRoute }} >
        {props.children}
    </Context.Provider>
}
