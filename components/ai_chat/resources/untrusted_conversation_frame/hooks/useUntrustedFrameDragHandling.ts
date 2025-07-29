/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import UntrustedConversationFrameAPI from '../untrusted_conversation_frame_api'

/**
 * Hook that detects drag start events in the untrusted iframe and
 * notifies the parent frame via mojom API.
 */
export function useUntrustedFrameDragHandling(): void {
  React.useEffect(() => {
    const api = UntrustedConversationFrameAPI.getInstance()

    const handleDragEnter = (e: DragEvent) => {
      if (e.dataTransfer?.types?.includes('Files')) {
        api.parentUIFrame.dragStart()
      }
    }

    document.addEventListener('dragenter', handleDragEnter)

    return () => {
      document.removeEventListener('dragenter', handleDragEnter)
    }
  }, [])
}
