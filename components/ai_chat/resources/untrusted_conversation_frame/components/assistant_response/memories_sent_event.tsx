// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Collapse from '@brave/leo/react/collapse'
import Icon from '@brave/leo/react/icon'
import { getLocale, formatLocale } from '$web-common/locale'
import { mojoTimeToJSDate } from '$web-common/mojomUtils'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './memories_sent_event.module.scss'
import '../../../common/strings'

interface Props {
  event: Mojom.MemoriesSentEvent
}

// The label of each field of the customization settings. The value goes in.
function getCustomizationLabel(kind: Mojom.SentMemoryKind) {
  switch (kind) {
    case Mojom.SentMemoryKind.kName:
      return S.CHAT_UI_MEMORIES_SENT_NAME_LABEL
    case Mojom.SentMemoryKind.kJob:
      return S.CHAT_UI_MEMORIES_SENT_JOB_LABEL
    case Mojom.SentMemoryKind.kTone:
      return S.CHAT_UI_MEMORIES_SENT_TONE_LABEL
    case Mojom.SentMemoryKind.kOtherInstructions:
      return S.CHAT_UI_MEMORIES_SENT_OTHER_LABEL
    default:
      return undefined
  }
}

function MemoryItem(props: { memory: Mojom.SentMemory }) {
  const { memory } = props
  const customizationLabel = getCustomizationLabel(memory.kind)
  return (
    <li>
      {customizationLabel !== undefined
        ? formatLocale(customizationLabel, { $1: memory.text })
        : memory.text}
      {memory.date && (
        <span className={styles.date}>
          {formatLocale(S.CHAT_UI_MEMORIES_SENT_LAST_MENTIONED, {
            $1: mojoTimeToJSDate(memory.date).toLocaleDateString(undefined, {
              dateStyle: 'medium',
            }),
          })}
        </span>
      )}
    </li>
  )
}

function MemoryGroup(props: { heading: string; memories: Mojom.SentMemory[] }) {
  if (props.memories.length === 0) {
    return null
  }
  return (
    <section>
      <h4>{props.heading}</h4>
      <ul>
        {props.memories.map((memory, i) => (
          <MemoryItem
            key={i}
            memory={memory}
          />
        ))}
      </ul>
    </section>
  )
}

/**
 * Shows the memories that Leo sent with the request for this reply: the
 * customization, the memories the user wrote, and the memories Leo learned. It
 * is closed until the user opens it. It only reads: the memories are managed in
 * the settings.
 */
export default function MemoriesSentEvent(props: Props) {
  const context = useUntrustedConversationContext()
  const [isOpen, setIsOpen] = React.useState(false)
  const { memories } = props.event

  const byKind = (kinds: Mojom.SentMemoryKind[]) =>
    memories.filter((memory) => kinds.includes(memory.kind))

  if (memories.length === 0) {
    return null
  }

  return (
    <div
      className={styles.memoriesSent}
      data-testid='memories-sent-event'
    >
      <Collapse
        isOpen={isOpen}
        onToggle={({ open }) => setIsOpen(open)}
      >
        <div slot='icon'>
          <Icon name='database' />
        </div>
        <div
          slot='title'
          className={styles.title}
        >
          {formatLocale(S.CHAT_UI_MEMORIES_SENT_TITLE, { $1: memories.length })}
        </div>
        <div className={styles.content}>
          <p>{getLocale(S.CHAT_UI_MEMORIES_SENT_DESCRIPTION)}</p>
          <MemoryGroup
            heading={getLocale(S.CHAT_UI_MEMORIES_SENT_CUSTOMIZATION_HEADING)}
            memories={byKind([
              Mojom.SentMemoryKind.kName,
              Mojom.SentMemoryKind.kJob,
              Mojom.SentMemoryKind.kTone,
              Mojom.SentMemoryKind.kOtherInstructions,
            ])}
          />
          <MemoryGroup
            heading={getLocale(S.CHAT_UI_MEMORIES_SENT_WRITTEN_HEADING)}
            memories={byKind([Mojom.SentMemoryKind.kWritten])}
          />
          <MemoryGroup
            heading={getLocale(S.CHAT_UI_MEMORIES_SENT_LEARNED_HEADING)}
            memories={byKind([Mojom.SentMemoryKind.kLearned])}
          />
          <div className={styles.actions}>
            <Button
              className={styles.manageButton}
              kind='plain-faint'
              size='small'
              onClick={() =>
                context.uiHandler?.openAIChatCustomizationSettings?.()
              }
              data-testid='memories-sent-manage-button'
            >
              {getLocale(S.CHAT_UI_MEMORIES_SENT_MANAGE_BUTTON_LABEL)}
            </Button>
          </div>
        </div>
      </Collapse>
    </div>
  )
}
