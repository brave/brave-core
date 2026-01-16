/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import UntrustedConversationFrameAPI from '../untrusted_conversation_frame_api'

/**
 * Sets up selection edge scroll handling for the untrusted iframe.
 * When the user is selecting text, this sends mouse position updates to the
 * parent frame so it can determine if scrolling is needed based on its own
 * visible viewport boundaries.
 */
export function selectionEdgeScrollSetup(): void {
  const api = UntrustedConversationFrameAPI.getInstance()

  let isSelecting = false

  const handleMouseMove = (e: MouseEvent) => {
    // Check if user is actively selecting text
    const selection = globalThis.getSelection()
    if (selection?.type !== 'Range') {
      if (isSelecting) {
        isSelecting = false
        api.parentUIFrame.selectionEnded()
      }
      return
    }

    // Check if mouse button is pressed (left button = 1 in e.buttons bitmask)
    if (!(e.buttons & 1)) {
      if (isSelecting) {
        isSelecting = false
        api.parentUIFrame.selectionEnded()
      }
      return
    }

    isSelecting = true

    // Send mouse position to parent - let parent determine if near edge
    const mouseY = e.clientY
    const iframeHeight = document.documentElement.scrollHeight

    api.parentUIFrame.selectionMouseMove(mouseY, iframeHeight)
  }

  const handleMouseUp = () => {
    if (isSelecting) {
      isSelecting = false
      api.parentUIFrame.selectionEnded()
    }
  }

  const handleSelectionChange = () => {
    const selection = globalThis.getSelection()
    if (selection?.type !== 'Range' && isSelecting) {
      isSelecting = false
      api.parentUIFrame.selectionEnded()
    }
  }

  document.addEventListener('mousemove', handleMouseMove)
  document.addEventListener('mouseup', handleMouseUp)
  document.addEventListener('selectionchange', handleSelectionChange)
}
