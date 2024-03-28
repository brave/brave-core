// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// MediaQueryList Event based useEventListener interface
function useEventListener<K extends keyof MediaQueryListEventMap>(
  eventName: K,
  handler: (event: MediaQueryListEventMap[K]) => void,
  element: React.RefObject<MediaQueryList>,
  options?: boolean | AddEventListenerOptions
): void

// Window Event based useEventListener interface
function useEventListener<K extends keyof WindowEventMap>(
  eventName: K,
  handler: (event: WindowEventMap[K]) => void,
  element?: undefined,
  options?: boolean | AddEventListenerOptions
): void

// Element Event based useEventListener interface
function useEventListener<
  K extends keyof HTMLElementEventMap & keyof SVGElementEventMap,
  T extends Element = K extends keyof HTMLElementEventMap
    ? HTMLDivElement
    : SVGElement
>(
  eventName: K,
  handler:
    | ((event: HTMLElementEventMap[K]) => void)
    | ((event: SVGElementEventMap[K]) => void),
  element: React.RefObject<T>,
  options?: boolean | AddEventListenerOptions
): void

// Document Event based useEventListener interface
function useEventListener<K extends keyof DocumentEventMap>(
  eventName: K,
  handler: (event: DocumentEventMap[K]) => void,
  element: React.RefObject<Document>,
  options?: boolean | AddEventListenerOptions
): void

function useEventListener<
  KW extends keyof WindowEventMap,
  KH extends keyof HTMLElementEventMap & keyof SVGElementEventMap,
  KM extends keyof MediaQueryListEventMap,
  T extends HTMLElement | SVGAElement | MediaQueryList = HTMLElement
>(
  eventName: KW | KH | KM,
  handler: (
    event:
      | WindowEventMap[KW]
      | HTMLElementEventMap[KH]
      | SVGElementEventMap[KH]
      | MediaQueryListEventMap[KM]
      | Event
  ) => void,
  element?: React.RefObject<T>,
  options?: boolean | AddEventListenerOptions
) {
  // Create a ref that stores handler
  const savedHandler = React.useRef(handler)

  React.useLayoutEffect(() => {
    savedHandler.current = handler
  }, [handler])

  React.useEffect(() => {
    // Define the listening target
    const targetElement: T | Window = element?.current ?? window

    if (!(targetElement && targetElement.addEventListener)) return

    // Create event listener that calls handler function stored in ref
    const listener: typeof handler = (event) => {
      savedHandler.current(event)
    }

    targetElement.addEventListener(eventName, listener, options)

    // Remove event listener on cleanup
    return () => {
      targetElement.removeEventListener(eventName, listener, options)
    }
  }, [eventName, element, options])
}

export { useEventListener }
