// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { WalletCardIds } from '$wallet/constants/types'

// Card Hub keeps the stack mounted and animates cards between three layouts:
//   1. Hub stack — cards sit in their stacked peek positions.
//   2. Details — other cards fade out, then the selected card slides into the
//      details slot.
//   3. Overlay screens (Settings, etc.) — the stack stays under the overlay so
//      it can shuffle back up when that screen closes.
//
// Motion is driven by `--stack-shift` / inline `transform`, plus WAAPI for the
// multi-keyframe close on the selected card. Cards are parked off-screen
// *before* the overlay unmounts so the first paint is already at the bottom.

// How long other cards fade before the selected card starts moving into details.
// Matches `leo.duration.m`.
const stackFadeMs = 200
// How long each card takes to rise from the bottom into its stack slot.
const stackReturnMs = 190
// Stagger between cards so the rise reads as 1, then 2, then 3.
const stackShuffleDelayMs = 80
// Ease-out so cards settle into place without a bounce.
const stackMoveEase = 'cubic-bezier(0, 0, 0.58, 1)'

const prefersReducedMotion = () => {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

// Total time until the last staggered card finishes. Used so the selected
// card's close path and the cleanup timer end with the shuffle.
const getShuffleDurationMs = (cardCount: number) => {
  if (cardCount <= 1) {
    return stackReturnMs
  }
  return (cardCount - 1) * stackShuffleDelayMs + stackReturnMs
}

// Drop WAAPI / inline motion so CSS can take over again (hover peek, etc.).
const clearCardMotionStyles = (item: HTMLDivElement) => {
  item.style.transition = ''
  item.style.transform = ''
  item.style.removeProperty('--stack-shift')
}

// Read an already-applied "parked at the bottom" offset. Used so a second
// park pass (layout effect after `onEnterHub`) does not snap cards back to
// rest and remeasure — that flash is what we saw when leaving Settings.
const readParkedRise = (item: HTMLDivElement) => {
  const fromVar = Number.parseFloat(
    item.style.getPropertyValue('--stack-shift'),
  )
  if (Number.isFinite(fromVar) && fromVar > 0) {
    return fromVar
  }
  const match = /translateY\((-?\d+(?:\.\d+)?)px\)/.exec(item.style.transform)
  return match ? Number(match[1]) : 0
}

// Instantly place cards below the hub (no transition). Must be a committed
// style, not only a WAAPI keyframe — otherwise the first frame paints them
// in the stack, then they jump down and rise.
const parkCardsAtBottom = (
  cards: HTMLDivElement[],
  bounds: DOMRect | undefined,
) => {
  const screenBottom = bounds?.bottom ?? window.innerHeight
  const parked = cards.map((item) => readParkedRise(item))
  if (parked.every((rise) => rise > 0)) {
    return parked
  }

  // Measure from rest so rise = distance from each card's stack slot to
  // the bottom of the hub wrapper.
  cards.forEach((item) => {
    item.style.transition = 'none'
    item.style.transform = 'none'
    item.style.setProperty('--stack-shift', '0px')
  })
  const rises = cards.map((item) => {
    return Math.max(screenBottom - item.getBoundingClientRect().top, 0)
  })
  cards.forEach((item, index) => {
    item.style.transition = 'none'
    item.style.transform = `translateY(${rises[index]}px)`
    item.style.setProperty('--stack-shift', `${rises[index]}px`)
  })
  // Flush layout so the parked transform is the current computed style
  // before WAAPI starts.
  cards[0]?.getBoundingClientRect()
  return rises
}

// 1-2-3 shuffle: each card has the same short rise, started `index * delay`
// later.
const shuffleCardsFromBottom = (
  cards: HTMLDivElement[],
  bounds: DOMRect | undefined,
) => {
  const rises = parkCardsAtBottom(cards, bounds)
  return cards.map((item, index) => {
    const rise = rises[index]
    const delay = index * stackShuffleDelayMs
    return item.animate(
      [
        { transform: `translateY(${rise}px)` },
        { transform: `translateY(0px)` },
      ],
      {
        duration: stackReturnMs,
        delay,
        easing: stackMoveEase,
        fill: 'forwards',
      },
    )
  })
}

// After the other cards have faded, slide the selected card so its top
// matches the invisible details slot.
const dockCardToDetailsSlot = (card: HTMLDivElement, slot: HTMLDivElement) => {
  const dy = slot.getBoundingClientRect().top - card.getBoundingClientRect().top

  if (prefersReducedMotion()) {
    card.style.setProperty('--stack-shift', `${dy}px`)
    return
  }

  card.style.setProperty('--stack-shift', '0px')
  const timeout = setTimeout(() => {
    card.style.setProperty('--stack-shift', `${dy}px`)
  }, stackFadeMs)
  return () => clearTimeout(timeout)
}

const cancelAnimations = (animations: Animation[]) => {
  animations.forEach((animation) => animation.cancel())
}

// Commit the last frame to inline styles, then cancel. Cancel-only would
// revert to the parked / docked transform and leave cards stuck mid-move.
const finishAnimations = (animations: Animation[]) => {
  animations.forEach((animation) => {
    try {
      animation.commitStyles()
    } catch {
      // Animation may already be cancelled.
    }
    animation.cancel()
  })
}

// Close details: unselected cards shuffle up from the bottom; the selected
// card lifts a bit more, then drops into its stack slot. Both paths use
// `closeMs` so the selected card finishes with the last shuffled card.
const returnCardsToStack = (
  selectedCard: HTMLDivElement,
  returningCards: HTMLDivElement[],
  bounds: DOMRect | undefined,
  onComplete: () => void,
) => {
  const screenTop = bounds?.top ?? 0
  const closeMs = getShuffleDurationMs(returningCards.length)
  // `--stack-shift` is still the docked details offset from the open.
  const startShift =
    Number.parseFloat(selectedCard.style.getPropertyValue('--stack-shift')) || 0
  // Extra lift so the selected card goes up toward the header before
  // falling into the stack. Floor of 80px keeps a short card visible.
  const extraUp = Math.max(
    selectedCard.getBoundingClientRect().bottom - screenTop - 70,
    80,
  )
  const peakShift = startShift - extraUp

  selectedCard.style.transition = 'none'
  const selectedAnimation = selectedCard.animate(
    [
      { transform: `translateY(${startShift}px)` },
      { transform: `translateY(${peakShift}px)`, offset: 0.3 },
      { transform: `translateY(0px)` },
    ],
    {
      duration: closeMs,
      easing: stackMoveEase,
      fill: 'forwards',
    },
  )
  const shuffleAnimations = shuffleCardsFromBottom(returningCards, bounds)
  const animations = [selectedAnimation, ...shuffleAnimations]

  // `completed` skips cancel-on-cleanup after a normal finish. The layout
  // effect re-runs when we clear `isClosingDetails`; cancelling then would
  // commit the start keyframe and leave cards in the wrong place.
  let completed = false
  const timeout = setTimeout(() => {
    completed = true
    finishAnimations(animations)
    onComplete()
  }, closeMs)
  return () => {
    clearTimeout(timeout)
    if (!completed) {
      cancelAnimations(animations)
    }
  }
}

export const useCardHubTransition = (visibleCardIds: WalletCardIds[]) => {
  // `selectedCardId` stays set for the whole close so the same DOM node
  // can animate from the details slot back into the stack.
  const [selectedCardId, setSelectedCardId] = React.useState<WalletCardIds>()
  const [isClosingDetails, setIsClosingDetails] = React.useState(false)
  // True while returning from Settings (or any future overlay screen).
  const [isEnteringHub, setIsEnteringHub] = React.useState(false)

  const stackItemRefs = React.useRef<Map<WalletCardIds, HTMLDivElement>>(
    new Map(),
  )
  // Invisible target in CardDetails; the selected card is measured against it.
  const detailsSlotRef = React.useRef<HTMLDivElement>(null)
  // Hub bounds — shuffle rise is to this bottom, not the viewport, so
  // Storybook / panel chrome does not throw the distance off.
  const wrapperRef = React.useRef<HTMLDivElement>(null)

  const detailsOpen = !!selectedCardId && !isClosingDetails
  // Wrapper clips overflow while cards are below the fold.
  const isStackAnimating = isClosingDetails || isEnteringHub

  const getVisibleCardElements = React.useCallback(() => {
    return visibleCardIds.flatMap((id) => {
      const item = stackItemRefs.current.get(id)
      return item ? [item] : []
    })
  }, [visibleCardIds])

  const setStackItemRef = React.useCallback(
    (id: WalletCardIds) => (element: HTMLDivElement | null) => {
      if (element) {
        stackItemRefs.current.set(id, element)
      } else {
        stackItemRefs.current.delete(id)
      }
    },
    [],
  )

  const resetCardMotion = React.useCallback(() => {
    stackItemRefs.current.forEach(clearCardMotionStyles)
  }, [])

  const onOpenCardDetails = React.useCallback((id: WalletCardIds) => {
    setIsClosingDetails(false)
    setIsEnteringHub(false)
    setSelectedCardId(id)
  }, [])

  const finishClosingDetails = React.useCallback(() => {
    resetCardMotion()
    setSelectedCardId(undefined)
    setIsClosingDetails(false)
  }, [resetCardMotion])

  const finishEnteringHub = React.useCallback(() => {
    resetCardMotion()
    setIsEnteringHub(false)
  }, [resetCardMotion])

  const onBackFromDetails = React.useCallback(() => {
    if (!selectedCardId) {
      return
    }
    if (prefersReducedMotion()) {
      finishClosingDetails()
      return
    }
    setIsClosingDetails(true)
  }, [finishClosingDetails, selectedCardId])

  // Call this *before* hiding an overlay. Parking while Settings still
  // covers the stack means the first frame after unmount is already
  // off-screen, not the rest position.
  const onEnterHub = React.useCallback(() => {
    if (prefersReducedMotion()) {
      return
    }
    parkCardsAtBottom(
      getVisibleCardElements(),
      wrapperRef.current?.getBoundingClientRect(),
    )
    setIsEnteringHub(true)
  }, [getVisibleCardElements])

  // Layout (not paint) effect: apply transforms before the browser draws,
  // so we never flash the rest stack. Three branches:
  //   entering hub  → shuffle every visible card up
  //   closing details → selected card up-then-down, others shuffle up
  //   opening details → wait for fade, then dock the selected card
  React.useLayoutEffect(() => {
    if (isEnteringHub) {
      const cards = getVisibleCardElements()
      if (cards.length === 0) {
        finishEnteringHub()
        return
      }
      const durationMs = getShuffleDurationMs(cards.length)
      const animations = shuffleCardsFromBottom(
        cards,
        wrapperRef.current?.getBoundingClientRect(),
      )
      let completed = false
      const timeout = setTimeout(() => {
        completed = true
        finishAnimations(animations)
        finishEnteringHub()
      }, durationMs)
      return () => {
        clearTimeout(timeout)
        if (!completed) {
          cancelAnimations(animations)
        }
      }
    }

    if (!selectedCardId) {
      return
    }
    const card = stackItemRefs.current.get(selectedCardId)
    const slot = detailsSlotRef.current
    if (!card || !slot) {
      return
    }

    if (isClosingDetails) {
      const returningCards = getVisibleCardElements().filter((item) => {
        return item !== card
      })
      return returnCardsToStack(
        card,
        returningCards,
        wrapperRef.current?.getBoundingClientRect(),
        finishClosingDetails,
      )
    }

    return dockCardToDetailsSlot(card, slot)
  }, [
    selectedCardId,
    isClosingDetails,
    isEnteringHub,
    finishClosingDetails,
    finishEnteringHub,
    getVisibleCardElements,
  ])

  return {
    selectedCardId,
    detailsOpen,
    isClosingDetails,
    isEnteringHub,
    isStackAnimating,
    wrapperRef,
    detailsSlotRef,
    setStackItemRef,
    onOpenCardDetails,
    onBackFromDetails,
    onEnterHub,
  }
}
