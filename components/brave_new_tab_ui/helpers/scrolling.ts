// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { debounce } from '../../common/debounce'

const overflowScrollableRegex = /(auto)|(scroll)/g
const isScrollable = (element: Element) => {
    return overflowScrollableRegex.test(getComputedStyle(element).overflow)
}

interface Options {
    limit: number
}

export const getScrollableParents = (element: Element | null | undefined, options?: Options) => {
    const scrollableElements: Element[] = []

    while (element && (!options?.limit || scrollableElements.length < options.limit)) {
        if (isScrollable(element)) { scrollableElements.push(element) }
        element = element.parentElement
    }

    return scrollableElements
}

export const getScrollableParent = (element: Element | null | undefined) => getScrollableParents(element, { limit: 1 })[0]

export const useParentScrolled = (element: HTMLElement | null, handler: (e: Event) => void) => {
    React.useEffect(() => {
        const scrollable = getScrollableParent(element)
        if (!scrollable) return

        scrollable.addEventListener('scroll', handler)
        return () => {
            scrollable.removeEventListener('scroll', handler)
        }
    }, [handler, element])
}

export const useMaintainScrollPosition = (localStorageKey: string, elementRef: React.MutableRefObject<HTMLElement | undefined>, bufferRate = 200) => {
    React.useEffect(() => {
        if (!elementRef.current) return

        const debouncedSave = debounce(() => {
            window.localStorage.setItem(localStorageKey, JSON.stringify({ top: elementRef.current?.scrollTop, left: elementRef.current?.scrollLeft }))
        }, bufferRate)

        const handler = () => {
            debouncedSave()
        }

        elementRef.current.addEventListener('scroll', handler)

        return () => {
            elementRef.current?.removeEventListener('scroll', handler)
        }
    }, [elementRef, localStorageKey, bufferRate])

    React.useEffect(() => {
        if (!elementRef.current) return

        const scrollPosition: { top: number, left: number } | null = JSON.parse(localStorage.getItem(localStorageKey) ?? 'null')
        if (!scrollPosition) return
        elementRef.current.scrollTo(scrollPosition)
    }, [elementRef, localStorageKey])
}
