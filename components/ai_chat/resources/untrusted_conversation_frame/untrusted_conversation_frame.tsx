// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-components/app.global.scss'
import '$web-common/defaultTrustedTypesPolicy'
import ConversationEntries from './components/conversation_entries'
import { UntrustedConversationContextProvider } from './untrusted_conversation_context'

import '../common/strings'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App() {
  // Iframe drag detection with proper cleanup
  React.useEffect(() => {
    let dragCounter = 0

    const handleDragEnter = (e: DragEvent) => {
      if (e.dataTransfer?.types?.includes('Files')) {
        dragCounter++
        if (dragCounter === 1) {
          window.parent.postMessage({ type: 'IFRAME_DRAG_START' }, '*')
        }
      }
    }

    const handleDragLeave = (e: DragEvent) => {
      if (e.dataTransfer?.types?.includes('Files')) {
        dragCounter--
        if (dragCounter === 0) {
          window.parent.postMessage({ type: 'IFRAME_DRAG_END' }, '*')
        }
      }
    }

    const handleDragEnd = (e: DragEvent) => {
      dragCounter = 0
      window.parent.postMessage({ type: 'IFRAME_DRAG_END' }, '*')
    }

    const handleDragOver = (e: DragEvent) => {
      e.preventDefault()
    }

    const handleDrop = (e: DragEvent) => {
      // Don't prevent default - let event bubble to parent overlay
      // for file processing
      dragCounter = 0
      window.parent.postMessage({ type: 'IFRAME_DRAG_END' }, '*')
    }

    // Note: Minimal drop handler to clean up drag state,
    // but let parent handle files

    document.addEventListener('dragenter', handleDragEnter)
    document.addEventListener('dragleave', handleDragLeave)
    document.addEventListener('dragover', handleDragOver)
    document.addEventListener('dragend', handleDragEnd)
    document.addEventListener('drop', handleDrop)

    return () => {
      document.removeEventListener('dragenter', handleDragEnter)
      document.removeEventListener('dragleave', handleDragLeave)
      document.removeEventListener('dragover', handleDragOver)
      document.removeEventListener('dragend', handleDragEnd)
      document.removeEventListener('drop', handleDrop)
    }
  }, [])

  return (
    <UntrustedConversationContextProvider>
      <ConversationEntries />
    </UntrustedConversationContextProvider>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
