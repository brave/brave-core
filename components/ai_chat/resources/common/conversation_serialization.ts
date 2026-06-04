// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

// TODO: get current client version from loadTimeDatas
export const CONVERSATION_EXPORT_VERSION = "1.93.8"

export interface SerializedConversation {
  version: string
  conversation: unknown[]
}

// Mojom conversation turns contain bigint fields (e.g. mojo.Time
// `internalValue`), which JSON cannot represent natively. We encode every
// bigint as a small tagged object so the value round-trips losslessly even
// when it exceeds Number.MAX_SAFE_INTEGER.
const BIGINT_TAG = '$bigint'

function bigIntReplacer(_key: string, value: unknown) {
  return typeof value === 'bigint' ? { [BIGINT_TAG]: value.toString() } : value
}

function bigIntReviver(_key: string, value: unknown) {
  if (
    value
    && typeof value === 'object'
    && BIGINT_TAG in value
    && Object.keys(value).length === 1
  ) {
    return BigInt((value as Record<string, string>)[BIGINT_TAG])
  }
  return value
}

/**
 * Serializes a conversation history to a JSON string that is compatible with
 * `displayConversation()` on the shared conversation page (see
 * `deserializeConversation`).
 */
export function serializeConversation(
  conversation: Mojom.ConversationTurn[],
): string {
  const payload: SerializedConversation = {
    version: CONVERSATION_EXPORT_VERSION,
    conversation,
  }
  return JSON.stringify(payload, bigIntReplacer, 2)
}

/**
 * Parses a JSON string produced by `serializeConversation` back into mojom
 * conversation turns (restoring bigint fields).
 */
export function deserializeConversation(
  json: string,
): Mojom.ConversationTurn[] {
  const payload = JSON.parse(json, bigIntReviver) as SerializedConversation
  if (!payload || !Array.isArray(payload.conversation)) {
    throw new Error('Invalid serialized conversation')
  }
  return payload.conversation as Mojom.ConversationTurn[]
}
