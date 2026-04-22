// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Shared in-memory registry of conversation metadata.
// bind_app_conversation.ts calls notifyConversationUpdated when turns change;
// bind_app_services.ts subscribes to push list updates into the UI API.

import * as Mojom from '../../../common/mojom'
import * as IDB from './idb_store'

export const CONV_KEY_PREFIX = 'conv:'

const conversations = new Map<string, Mojom.Conversation>()
const subscribers = new Set<(convs: Mojom.Conversation[]) => void>()
let initialLoadDone = false
let initialLoadPromise: Promise<void> | undefined

function buildMeta(uuid: string, turns: Mojom.ConversationTurn[]): Mojom.Conversation {
  const firstHuman = turns.find(
    (t) => t.characterType === Mojom.CharacterType.HUMAN,
  )
  const lastTurn = turns[turns.length - 1]
  return {
    uuid,
    title: firstHuman?.text?.slice(0, 200) ?? '',
    updatedTime: lastTurn?.createdTime ?? { internalValue: BigInt(0) },
    hasContent: turns.length > 0,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
    associatedContent: [],
  }
}

function sorted(): Mojom.Conversation[] {
  return [...conversations.values()].sort((a, b) => {
    const diff = b.updatedTime.internalValue - a.updatedTime.internalValue
    return diff > BigInt(0) ? 1 : diff < BigInt(0) ? -1 : 0
  })
}

function broadcast() {
  const list = sorted()
  for (const fn of subscribers) fn(list)
}

async function ensureLoaded() {
  if (initialLoadDone) return
  if (initialLoadPromise) return initialLoadPromise
  initialLoadPromise = (async () => {
    const allKeys = await IDB.keys()
    await Promise.all(
      allKeys
        .filter((k) => k.startsWith(CONV_KEY_PREFIX))
        .map(async (key) => {
          const uuid = key.slice(CONV_KEY_PREFIX.length)
          const turns = await IDB.get<Mojom.ConversationTurn[]>(key)
          if (turns?.length) {
            conversations.set(uuid, buildMeta(uuid, turns))
          }
        }),
    )
    initialLoadDone = true
  })()
  return initialLoadPromise
}

export async function getAllConversations(): Promise<Mojom.Conversation[]> {
  await ensureLoaded()
  return sorted()
}

export function notifyConversationUpdated(
  uuid: string,
  turns: Mojom.ConversationTurn[],
) {
  if (turns.length > 0) {
    conversations.set(uuid, buildMeta(uuid, turns))
  }
  broadcast()
}

export function notifyConversationDeleted(uuid: string) {
  conversations.delete(uuid)
  broadcast()
}

export function subscribeToConversationList(
  fn: (convs: Mojom.Conversation[]) => void,
): () => void {
  subscribers.add(fn)
  return () => subscribers.delete(fn)
}
