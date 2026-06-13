// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import * as Mojom from '../common/mojom'
import { deserializeConversation } from '../common/conversation_serialization'

interface DemoSharedConversationInputProps {
  onLoad: (conversation: Mojom.ConversationTurn[]) => void
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
  console.log('demo')
  const { onLoad } = props
  const [value, setValue] = React.useState('')
  const [error, setError] = React.useState<string | null>(null)
  const [submitted, setSubmitted] = React.useState(false)

  const handleLoad = React.useCallback(() => {
    try {
      const conversation = deserializeConversation(value)
      setError(null)
      onLoad(conversation)
      setSubmitted(true)
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
    }
  }, [value, onLoad])

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
          fontFamily: 'var(--leo-font-mono, monospace)',
          fontSize: '12px',
          padding: 'var(--leo-spacing-m)',
          borderRadius: 'var(--leo-radius-m)',
          border: '1px solid var(--leo-color-divider-subtle)',
          background: 'var(--leo-color-container-background)',
          color: 'var(--leo-color-text-primary)',
          boxSizing: 'border-box',
        }}
      />
      {error && (
        <div style={{ color: 'var(--leo-color-systemfeedback-error-icon)' }}>
          {error}
        </div>
      )}
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
