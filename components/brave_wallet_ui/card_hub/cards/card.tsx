// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styles
import { CardFace, StyledCard } from './cards.style'

// Constants
const maxTiltDeg = 12
const maxRotateDeg = 6
const maxShadowPx = 16

const prefersReducedMotion = () => {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

interface Props {
  children: React.ReactNode
  onClick?: () => void
}

export const Card = (props: Props) => {
  const { children, onClick } = props

  // Refs
  const cardRef = React.useRef<HTMLDivElement>(null)

  // Methods
  const resetTilt = React.useCallback(() => {
    const card = cardRef.current
    if (!card) {
      return
    }
    card.style.setProperty('--tilt-x', '0deg')
    card.style.setProperty('--tilt-y', '0deg')
    card.style.setProperty('--rotate-z', '0deg')
    card.style.setProperty('--shadow-x', '0px')
    card.style.setProperty('--shadow-y', '4px')
  }, [])

  const onMouseMove = React.useCallback(
    (event: React.MouseEvent<HTMLDivElement>) => {
      const card = cardRef.current
      if (!card || prefersReducedMotion()) {
        return
      }

      const rect = card.getBoundingClientRect()
      if (rect.width === 0 || rect.height === 0) {
        return
      }

      const px = (event.clientX - rect.left) / rect.width
      const py = (event.clientY - rect.top) / rect.height
      // Tilt the hovered edge toward the cursor and twist slightly with it.
      const tiltX = (py - 0.5) * maxTiltDeg
      const tiltY = (0.5 - px) * maxTiltDeg
      const rotateZ = (0.5 - px) * maxRotateDeg
      // Cast the shadow opposite the raised edge.
      const shadowX = (0.5 - px) * maxShadowPx
      const shadowY = 12 + (0.5 - py) * 8
      card.style.setProperty('--tilt-x', `${tiltX.toFixed(2)}deg`)
      card.style.setProperty('--tilt-y', `${tiltY.toFixed(2)}deg`)
      card.style.setProperty('--rotate-z', `${rotateZ.toFixed(2)}deg`)
      card.style.setProperty('--shadow-x', `${shadowX.toFixed(1)}px`)
      card.style.setProperty('--shadow-y', `${shadowY.toFixed(1)}px`)
    },
    [],
  )

  return (
    <StyledCard
      ref={cardRef}
      role='button'
      onMouseMove={onMouseMove}
      onMouseLeave={resetTilt}
      onClick={onClick}
    >
      <CardFace>{children}</CardFace>
    </StyledCard>
  )
}
