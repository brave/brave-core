/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import UntrustedConversationFrameAPI from '../untrusted_conversation_frame_api'

/**
 * Sets up drag handling for the untrusted iframe.
 * Detects drag start events and notifies the parent frame via mojom API.
 */
export function untrustedFrameDragHandlingSetup(): void {
  const api = UntrustedConversationFrameAPI.getInstance()

  const handleDragEnter = (e: DragEvent) => {
    if (e.dataTransfer?.types?.includes('Files')) {
      api.parentUIFrame.dragStart()
    }
  }

  document.addEventListener('dragenter', handleDragEnter)
}
