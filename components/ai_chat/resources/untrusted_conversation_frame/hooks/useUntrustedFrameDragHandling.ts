/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

/**
 * Sets up drag handling for the untrusted iframe.
 * Detects drag start events and notifies the parent frame via the provided callback.
 * Should be called early in module initialization.
 */
export function useUntrustedFrameDragHandling(onDragStarted: () => void): void {
  React.useEffect(() => {
    const handleDragEnter = (e: DragEvent) => {
      const types = e.dataTransfer?.types ?? []
      const isFileDrag = types.includes('Files')
      // Heuristic for a web image drag: the browser puts text/uri-list
      // (the image URL) and text/html (the <img> tag) in the transfer.
      // getData() is not readable during dragenter, so we check types only.
      const isWebImageDrag =
        types.includes('text/uri-list') && types.includes('text/html')
      if (isFileDrag || isWebImageDrag) {
        onDragStarted()
      }
    }

    document.addEventListener('dragenter', handleDragEnter)

    return () => {
      document.removeEventListener('dragenter', handleDragEnter)
    }
  }, [onDragStarted])
}
