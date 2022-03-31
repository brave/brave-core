// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

let instanceCount = 0

export default function useTriggerOnNearViewport (handlerRef: React.MutableRefObject<Function>) {
  // When we're intersecting for the first time, ask for an Ad
  const hasPassedAdRequestThreshold = React.useRef(false)
  // Element which will be observed to trigger ad fetch
  const contentTrigger = React.useRef<HTMLDivElement>(null)
  // Observer which will observe elements scroll position to trigger ad fetch
  const contentTriggerObserver = React.useRef<IntersectionObserver>()
  const debugInstanceId = React.useMemo(function () {
    return instanceCount++
  }, [])
  // Setup ad fetch trigger observer on first mount
  React.useEffect(() => {
    console.debug('Brave News: creating observer', debugInstanceId)
    contentTriggerObserver.current = new IntersectionObserver(async (entries) => {
      // Only request an ad the first time it passes threshold
      if (!hasPassedAdRequestThreshold.current && entries.some((entry) => entry.isIntersecting)) {
        hasPassedAdRequestThreshold.current = true
        console.debug('Brave News: asking for an ad', debugInstanceId)
        handlerRef.current && handlerRef.current()
      }
    }, {
      // Trigger ad fetch when the ad unit is 1000px away from the viewport
      rootMargin: '0px 0px 1000px 0px'
    })
  }, [])
  // Observe and disconnect from content trigger when it's appropriate
  React.useEffect(() => {
    if (hasPassedAdRequestThreshold.current || !contentTrigger.current || !contentTriggerObserver.current) {
      console.debug('Brave News: not creating trigger', debugInstanceId)
      return
    }
    const observer = contentTriggerObserver.current
    console.debug('Brave News: ad fetch trigger observer connected', debugInstanceId)
    observer.observe(contentTrigger.current)
    return () => {
      console.debug('Brave News: ad fetch trigger observer disconnected', debugInstanceId)
      observer.disconnect()
    }
  }, [
    // Disconnect when we have asked for an ad
    // (opportunistically at the next render as ref mutations don't cause effects to run)
    hasPassedAdRequestThreshold.current,
    // Observer or Disconnect when we have a new element (should have caused a render, so this should get run)
    contentTrigger.current,
    // Observe or Disconnect when we have a new intersection observer (should be at first mount)
    contentTriggerObserver.current
  ])
  return [contentTrigger]
}
