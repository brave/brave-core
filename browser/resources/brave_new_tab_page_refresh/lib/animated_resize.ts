/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const resizingAttribute = 'data-resizing'

const animationOptions: KeyframeAnimationOptions = {
  duration: 180,
  easing: 'ease-out',
}

function prefersReducedMotion() {
  return matchMedia('(prefers-reduced-motion: reduce)').matches
}

// Animates the height of an element when its content causes it to resize. The
// element carries a "data-resizing" attribute while the animation is running,
// which can be used to hide content that would otherwise overflow. Showing or
// hiding the element is not animated.
export function useAnimatedResize(ref: React.RefObject<HTMLElement | null>) {
  React.useEffect(() => {
    const elem = ref.current
    if (!elem) {
      return
    }

    let animation: Animation | null = null
    let height = elem.offsetHeight

    function onResize() {
      // The running animation is itself a source of resize notifications.
      if (animation || !elem) {
        return
      }

      const previousHeight = height
      height = elem.offsetHeight

      if (
        previousHeight === 0
        || height === 0
        || previousHeight === height
        || prefersReducedMotion()
      ) {
        return
      }

      animation = elem.animate(
        [{ height: `${previousHeight}px` }, { height: `${height}px` }],
        animationOptions,
      )

      elem.setAttribute(resizingAttribute, '')

      animation.onfinish = () => {
        animation = null
        elem.removeAttribute(resizingAttribute)
        onResize()
      }
    }

    const observer = new ResizeObserver(onResize)
    observer.observe(elem)

    return () => {
      observer.disconnect()
      animation?.cancel()
      elem.removeAttribute(resizingAttribute)
    }
  }, [ref])
}
