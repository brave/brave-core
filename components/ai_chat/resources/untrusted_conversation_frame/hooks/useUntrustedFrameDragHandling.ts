/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Mojom from '../../common/mojom'

// Store the callback to be called when drag starts
let dragStartCallback: (() => void) | undefined

/**
 * Sets up drag handling for the untrusted iframe.
 * Detects drag start events and notifies the parent frame via the provided callback.
 * Should be called early in module initialization.
 */
export function untrustedFrameDragHandlingSetup(): void {
  const handleDragEnter = (e: DragEvent) => {
    if (e.dataTransfer?.types?.includes('Files')) {
      dragStartCallback?.()
    }
  }

  document.addEventListener('dragenter', handleDragEnter)
}

/**
 * Registers the callback to be called when a file drag starts.
 * Should be called after the parent UI frame is bound.
 */
export function registerDragStartCallback(
  parentUIFrame: Mojom.ParentUIFrameInterface,
): void {
  dragStartCallback = () => parentUIFrame.dragStart()
}
