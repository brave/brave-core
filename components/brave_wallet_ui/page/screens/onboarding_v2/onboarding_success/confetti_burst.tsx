// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as leo from '@brave/nala/tokens/css/variables'

// Styled Components
import { ConfettiLayer, ConfettiPiece } from './onboarding_success.style'

const CONFETTI_COLORS = [
  leo.color.primitive.brands.rorange2,
  leo.color.primitive.brands.rorange3,
  leo.color.privateWindow['30'],
  leo.color.schemes.primary,
  leo.color.green['30'],
]

const prefersReducedMotion = () => {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

const createPieces = (count: number) => {
  return Array.from({ length: count }, (_, index) => {
    const angle = (index / count) * Math.PI * 2 + Math.random() * 0.4
    const distance = 90 + Math.random() * 90
    return {
      id: index,
      x: Math.cos(angle) * distance,
      y: Math.sin(angle) * distance * 0.8 + 30,
      rotate: Math.random() * 540 - 270,
      delay: Math.random() * 0.18,
      duration: 1.1 + Math.random() * 0.7,
      color: CONFETTI_COLORS[index % CONFETTI_COLORS.length],
      round: index % 3 === 0,
    }
  })
}

export const ConfettiBurst = ({ count = 26 }: { count?: number }) => {
  const pieces = React.useMemo(() => createPieces(count), [count])

  if (prefersReducedMotion()) {
    return null
  }

  return (
    <ConfettiLayer aria-hidden={true}>
      {pieces.map((piece) => (
        <ConfettiPiece
          key={piece.id}
          $x={piece.x}
          $y={piece.y}
          $rotate={piece.rotate}
          $delay={piece.delay}
          $duration={piece.duration}
          $round={piece.round}
          $color={piece.color}
        />
      ))}
    </ConfettiLayer>
  )
}
