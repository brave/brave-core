// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useState } from 'react'
import {
  MouseSensor,
  PointerSensor,
  TouchSensor,
  UniqueIdentifier,
  useSensor,
  useSensors
} from '@dnd-kit/core'
import { Transform , CSS } from '@dnd-kit/utilities'
import { useSortable } from '@dnd-kit/sortable'
import { Arguments } from '@dnd-kit/sortable/dist/hooks/useSortable'

export function useSensorsWithThreshold () {
  const dragThreshold = 10
  return useSensors(
    useSensor(MouseSensor, {
      activationConstraint: { distance: dragThreshold }
    }),
    useSensor(PointerSensor, {
      activationConstraint: { distance: dragThreshold }
    }),
    useSensor(TouchSensor, {
      activationConstraint: {
        delay: 250,
        tolerance: dragThreshold
      }
    })
  )
}

export const restrictToVerticalAxis = [
  ({ transform }: { transform: Transform }) => {
    return { ...transform, x: 0 }
  }
]

export function useDraggedId () {
  return useState<UniqueIdentifier | null>(null)
}

export function useDraggedOrder<T> () {
  return useState<T[] | null>(null)
}

export function useVerticallySortable ({ id }: Arguments) {
  const attributes = useSortable({ id })

  if (attributes.transform) attributes.transform.x = 0
  const style = {
    transform: CSS.Transform.toString(attributes.transform),
    transition: attributes.transition
  }

  return { ...attributes, style }
}
