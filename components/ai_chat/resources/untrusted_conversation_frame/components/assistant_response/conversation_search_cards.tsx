// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import '../../../common/strings'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './conversation_search_cards.module.scss'

interface ConversationCard {
  conversationUuid: string
  title: string
  entryUuid?: string
  snippet?: string
}

// Reads the cards of a conversation_search_results artifact, leaving out
// anything that isn't one.
export function parseConversationCards(
  artifact: Mojom.ToolArtifact,
): ConversationCard[] {
  if (artifact.type !== Mojom.CONVERSATION_SEARCH_RESULTS_ARTIFACT_TYPE) {
    return []
  }
  let parsed: unknown
  try {
    parsed = JSON.parse(artifact.contentJson)
  } catch {
    return []
  }
  if (!Array.isArray(parsed)) {
    return []
  }
  return parsed.flatMap((item): ConversationCard[] => {
    if (
      typeof item?.conversation_uuid !== 'string'
      || !item.conversation_uuid
      || typeof item.title !== 'string'
    ) {
      return []
    }
    return [
      {
        conversationUuid: item.conversation_uuid,
        title: item.title,
        entryUuid:
          typeof item.entry_uuid === 'string' && item.entry_uuid
            ? item.entry_uuid
            : undefined,
        snippet:
          typeof item.snippet === 'string' && item.snippet
            ? item.snippet
            : undefined,
      },
    ]
  })
}

interface Props {
  artifact: Mojom.ToolArtifact
}

// The conversations found by the conversation_search tool, as cards that open
// the conversation at the entry that matched.
export default function ConversationSearchCards({ artifact }: Props) {
  const context = useUntrustedConversationContext()
  const cards = React.useMemo(
    () => parseConversationCards(artifact),
    [artifact],
  )

  if (cards.length === 0) {
    return null
  }

  return (
    <ol
      className={styles.cards}
      data-testid='conversation-search-cards'
    >
      {cards.map((card) => (
        <li key={card.conversationUuid}>
          <button
            className={styles.card}
            type='button'
            data-testid='conversation-search-card'
            onClick={() =>
              // The frame can't open the conversation itself; the page that
              // shows it does.
              context.parentUiFrame.openConversation(
                card.conversationUuid,
                card.entryUuid ?? null,
              )
            }
          >
            <div
              className={styles.title}
              title={
                card.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)
              }
            >
              {card.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)}
            </div>
            {card.snippet && (
              <div
                className={styles.snippet}
                data-testid='conversation-search-card-snippet'
              >
                {card.snippet}
              </div>
            )}
          </button>
        </li>
      ))}
    </ol>
  )
}
