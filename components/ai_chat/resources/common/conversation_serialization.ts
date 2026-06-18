// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

// TODO(https://github.com/brave/brave-browser/issues/56444): get current client
// version from loadTimeData.
export const CONVERSATION_EXPORT_VERSION = '1.93.8'

type ConversationData = Mojom.ConversationTurn[]

export interface SerializedConversation {
  /**
   * The version of the Brave client that produced this serialized conversation.
   */
  version: string

  /**
   * A JSON string of ConversationData, which may contain bigint fields
   */
  data: string
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
 * Serializes all conversation data to a JSON string, which can be restored
 * with parseConversationData. This is a lossless serialization.
 */
export function stringifyConversationData(data: ConversationData): string {
  return JSON.stringify(data, bigIntReplacer)
}

/**
 * Parses a JSON string produced by stringifyConversationData and restores
 * the original conversation data, including bigint fields.
 */
export function parseConversationData(dataJson: string): ConversationData {
  return JSON.parse(dataJson, bigIntReviver) as ConversationData
}

/**
 * Serializes a conversation to a JSON string and wraps in a format that the
 * sharing viewer expects, including the UI version number which generated it.
 */
export function serializeConversationForSharing(
  data: ConversationData,
): string {
  // The payload is double-stringified so that the viewer can have a stable
  // format of { version, data }, and the versioned UI code can perform
  // the parsing of the conversation data, the above algorithm of which may
  // change over time, and will be deployed with that version of the UI code.
  const payload: SerializedConversation = {
    version: CONVERSATION_EXPORT_VERSION,
    data: stringifyConversationData(data),
  }

  return JSON.stringify(payload)
}
