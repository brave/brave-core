/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import getAPI from '../api'

interface IframeDragHandlers {
  setDragActive: (active: boolean) => void
  setDragOver: (over: boolean) => void
}

/**
 * Hook that handles iframe drag events via mojom API and updates drag state
 */
export function useIframeDragHandling({
  setDragActive,
  setDragOver
}: IframeDragHandlers) {
  React.useEffect(() => {
    const api = getAPI()

    const dragStartId = api.conversationEntriesFrameObserver.dragStart
      .addListener(() => {
        setDragActive(true)
        setDragOver(true)
      })

    return () => {
      api.conversationEntriesFrameObserver.removeListener(dragStartId)
    }
  }, [setDragActive, setDragOver])
}