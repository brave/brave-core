/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useAIChat } from '../state/ai_chat_context'

interface DocumentDragHandlers {
  setDragActive: (active: boolean) => void
  setDragOver: (over: boolean) => void
  clearDragState: () => void
}

/**
 * Hook that handles document-level and iframe drag events and updates drag
 * state
 */
export function useIsDragging({
  setDragActive,
  setDragOver,
  clearDragState,
}: DocumentDragHandlers) {
  // Iframe drag detection
  const aiChat = useAIChat()

  // Forward events from child frame
  aiChat.api.useDragStart(() => {
    setDragActive(true)
    setDragOver(true)
  })

  // Document-level drag detection
  React.useEffect(() => {
    // Track the number of dragenter/dragleave events to handle nested DOM
    // elements. When dragging over child elements, each element fires its own
    // enter/leave events, so we need a counter to only clear the drag state
    // when we've truly left the document.
    let dragCounter = 0
    let dragTimeoutId: number | null = null

    const clearDragStateInternal = () => {
      dragCounter = 0
      clearDragState()
      if (dragTimeoutId) {
        clearTimeout(dragTimeoutId)
        dragTimeoutId = null
      }
    }

    const handleDragEnter = (e: DragEvent) => {
      e.preventDefault()
      dragCounter++

      if (e.dataTransfer?.types?.includes('Files')) {
        setDragActive(true)
        setDragOver(true)

        // Set a timeout to clear drag state if no activity for 1 second
        if (dragTimeoutId) {
          clearTimeout(dragTimeoutId)
        }
        dragTimeoutId = window.setTimeout(() => {
          clearDragStateInternal()
        }, 1000)
      }
    }

    const handleDragLeave = (e: DragEvent) => {
      e.preventDefault()
      dragCounter--

      if (dragCounter === 0) {
        clearDragStateInternal()
      }
    }

    const handleDragOver = (e: DragEvent) => {
      e.preventDefault()
      // Reset timeout on any drag activity
      if (dragTimeoutId && e.dataTransfer?.types?.includes('Files')) {
        clearTimeout(dragTimeoutId)
        dragTimeoutId = window.setTimeout(() => {
          clearDragStateInternal()
        }, 1000)
      }
    }

    // Handle drag operation end or interruption
    const handleDragEnd = (e: DragEvent) => {
      clearDragStateInternal()
    }

    // Handle window focus loss (Alt+Tab, etc.)
    const handleWindowBlur = () => {
      clearDragStateInternal()
    }

    // Handle page visibility change
    const handleVisibilityChange = () => {
      if (document.hidden) {
        clearDragStateInternal()
      }
    }

    document.addEventListener('dragenter', handleDragEnter)
    document.addEventListener('dragleave', handleDragLeave)
    document.addEventListener('dragover', handleDragOver)
    document.addEventListener('dragend', handleDragEnd)
    window.addEventListener('blur', handleWindowBlur)
    document.addEventListener('visibilitychange', handleVisibilityChange)

    return () => {
      if (dragTimeoutId) {
        clearTimeout(dragTimeoutId)
      }
      document.removeEventListener('dragenter', handleDragEnter)
      document.removeEventListener('dragleave', handleDragLeave)
      document.removeEventListener('dragover', handleDragOver)
      document.removeEventListener('dragend', handleDragEnd)
      window.removeEventListener('blur', handleWindowBlur)
      document.removeEventListener('visibilitychange', handleVisibilityChange)
    }
  }, [])
}
