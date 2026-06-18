// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import Button from '@brave/leo/react/button'
import { renderConversation } from './render_conversation'
import './demo.css'

interface DemoSharedConversationInputProps {
  onLoad: (conversationRaw: string) => void
}

/**
 * A development-only helper that lets you paste a conversation JSON export
 * (produced by the AI Chat page's "Export conversation" action) and render it.
 * This is not part of the shipped shared conversation experience - it only
 * exists to exercise displayConversation() with local data.
 */
export default function DemoSharedConversationInput(
  props: DemoSharedConversationInputProps,
) {
  const { onLoad } = props
  const [value, setValue] = React.useState('')
  const [submitted, setSubmitted] = React.useState(false)

  const handleLoad = () => {
    onLoad(value)
    setSubmitted(true)
  }

  // Hide the demo input once a conversation has been loaded.
  if (submitted) {
    return null
  }

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        gap: 'var(--leo-spacing-m)',
        padding: 'var(--leo-spacing-xl)',
        maxWidth: '720px',
        margin: '0 auto',
      }}
    >
      <label htmlFor='demo-conversation-json'>
        Paste an exported conversation JSON below and load it:
      </label>
      <textarea
        id='demo-conversation-json'
        value={value}
        onChange={(e) => setValue(e.currentTarget.value)}
        placeholder='{ "version": "1.92.408", "conversation": [ ... ] }'
        spellCheck={false}
        style={{
          width: '100%',
          minHeight: '180px',
          resize: 'vertical',
          fontFamily: 'var(--leo-font-monospace-default)',
          fontSize: '12px',
          padding: 'var(--leo-spacing-m)',
          borderRadius: 'var(--leo-radius-m)',
          border: '1px solid var(--leo-color-divider-subtle)',
          background: 'var(--leo-color-container-background)',
          color: 'var(--leo-color-text-primary)',
          boxSizing: 'border-box',
        }}
      />
      <div>
        <Button
          kind='filled'
          onClick={handleLoad}
          isDisabled={value.trim().length === 0}
        >
          Load saved conversation
        </Button>
      </div>
    </div>
  )
}

function handleConversationLoaded(conversation: any) {
  // Parse stable format, unlikely to change
  const sharedConversation = JSON.parse(conversation) as {
    version: string
    data: string
  }

  if (!sharedConversation.data || typeof sharedConversation.data !== 'string') {
    alert('Invalid shared conversation data')
    return
  }

  renderConversation(
    sharedConversation.data,
    document.getElementById('conversation')!,
  )
}

function initialize() {
  const root = createRoot(document.getElementById('demo')!)
  root.render(<DemoSharedConversationInput onLoad={handleConversationLoaded} />)
}

document.addEventListener('DOMContentLoaded', initialize)
