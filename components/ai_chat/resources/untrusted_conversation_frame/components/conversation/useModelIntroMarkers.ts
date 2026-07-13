// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import { getPairedConversationGroups } from '../conversation_entries/conversation_entries_utils'

/**
 * A sticky ModelIntro pill anchored to the point in the conversation where
 * the user selected a non-default model. Markers are session-only.
 *
 * - `afterPairIndex === null`: render at the top of Conversation (before
 *   any entries), typically when the model was changed before the first
 *   message.
 * - `afterPairIndex === N`: render inside ConversationEntries after entry
 *   pair N.
 */
export type ModelIntroMarker = {
  id: string
  modelKey: string
  afterPairIndex: number | null
}

let modelIntroMarkerCounter = 0

function createModelIntroMarker(
  modelKey: string,
  afterPairIndex: number | null,
): ModelIntroMarker {
  modelIntroMarkerCounter += 1
  return {
    id: `model-intro-${modelIntroMarkerCounter}`,
    modelKey,
    afterPairIndex,
  }
}

/**
 * Tracks append-only ModelIntro markers for the active conversation session.
 * Only used by Conversation (and passed down to ConversationEntries).
 */
export function useModelIntroMarkers(): ModelIntroMarker[] {
  const context = useUntrustedConversationContext()
  const state = context.api.useState().data
  const conversationHistory = context.api.useGetConversationHistoryData()

  const [modelIntroMarkers, setModelIntroMarkers] = React.useState<
    ModelIntroMarker[]
  >([])

  // Last observed currentModelKey. Used to distinguish the initial mount
  // (and post-reset baseline) from a real model change that should create a
  // marker. Avoids showing a pill for whatever model happens to be selected
  // when the conversation first loads.
  const prevModelKeyRef = React.useRef<string | undefined>(undefined)

  // Identity of the conversation currently showing markers. The untrusted
  // frame does not receive a conversation-level uuid on every history turn,
  // so we use the first turn's uuid as a stable stand-in. When it changes,
  // the user switched conversations and session markers must be cleared.
  const conversationIdentityRef = React.useRef<string | null>(null)

  // Reset markers when the conversation session ends or switches.
  React.useEffect(() => {
    // Empty history means a brand-new / cleared conversation: drop any
    // leftover markers and reset tracking so the next model selection can
    // create a fresh top-of-conversation marker.
    if (conversationHistory.length === 0) {
      setModelIntroMarkers([])
      conversationIdentityRef.current = null
      prevModelKeyRef.current = undefined
      return
    }

    const firstUuid = conversationHistory[0]?.uuid
    if (!firstUuid) {
      return
    }

    // First turn arrived for this session — record identity without clearing
    // markers so a top-of-conversation pill created before the first message
    // stays visible once the conversation starts.
    if (conversationIdentityRef.current === null) {
      conversationIdentityRef.current = firstUuid
      return
    }

    // Different first-turn uuid means a different conversation was loaded.
    // Clear session markers and allow the next model change to create new
    // ones from a clean baseline.
    if (conversationIdentityRef.current !== firstUuid) {
      conversationIdentityRef.current = firstUuid
      setModelIntroMarkers([])
      prevModelKeyRef.current = undefined
    }
  }, [conversationHistory.length, conversationHistory[0]?.uuid])

  // Append a marker when the user selects a non-default model.
  React.useEffect(() => {
    const { currentModelKey, defaultModelKey } = state

    // Establish a baseline on mount / after a reset; do not create a marker
    // for the model that is already selected.
    if (prevModelKeyRef.current === undefined) {
      prevModelKeyRef.current = currentModelKey
      return
    }

    if (prevModelKeyRef.current === currentModelKey) {
      return
    }

    prevModelKeyRef.current = currentModelKey

    // Switching back to the default model should not add a new pill;
    // existing historical markers remain.
    if (currentModelKey === defaultModelKey) {
      return
    }

    setModelIntroMarkers((previousMarkers) => {
      // Ignore duplicate consecutive selections of the same non-default model.
      const lastMarker = previousMarkers.at(-1)
      if (lastMarker?.modelKey === currentModelKey) {
        return previousMarkers
      }

      // Anchor to the latest entry pair, or to the top when there are no turns
      // yet (model changed before the conversation started).
      const pairedGroups = getPairedConversationGroups(conversationHistory)
      const afterPairIndex =
        conversationHistory.length === 0 ? null : pairedGroups.length - 1

      return [
        ...previousMarkers,
        createModelIntroMarker(currentModelKey, afterPairIndex),
      ]
    })
  }, [conversationHistory.length, state.currentModelKey, state.defaultModelKey])

  return modelIntroMarkers
}
