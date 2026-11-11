/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { OfferEventSpec, OfferEventTrigger } from '../../lib/app_store'
import { useAppActions } from '../../lib/app_context'

// Offers and event types reported in this page load.
const reported = new Set<string>()

// Reports the offer events that /v1/cards lists for an offer.
export function useOfferEvents(offerId: string, events: OfferEventSpec[]) {
  const actions = useAppActions()
  const timers = React.useRef<number[]>([])

  const cancel = () => {
    timers.current.forEach((id) => window.clearTimeout(id))
    timers.current = []
  }

  React.useEffect(() => cancel, [])

  const withTrigger = (trigger: OfferEventTrigger) =>
    offerId ? events.filter((spec) => spec.trigger === trigger) : []

  const report = (spec: OfferEventSpec) => {
    const key = `${offerId}/${spec.type}`
    if (spec.oncePerLoad && reported.has(key)) {
      return
    }
    reported.add(key)
    actions.recordOfferEvent(offerId, spec.type)
  }

  return {
    onPointerEnter: (event: React.PointerEvent) => {
      // Touch input has no hover state.
      if (event.pointerType !== 'mouse') {
        return
      }
      cancel()
      for (const spec of withTrigger('hover')) {
        timers.current.push(
          window.setTimeout(() => report(spec), spec.dwellMs),
        )
      }
    },
    onPointerLeave: cancel,
    reportClick: () => withTrigger('click').forEach(report),
  }
}
