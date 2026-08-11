// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable import/first */

// Set up loadTimeData mock BEFORE importing the module under test
;(window as any).loadTimeData = {
  getString: (key: string) => (key === 'braveVersion' ? '1.93.8' : ''),
}

import { describe, it, expect } from '@jest/globals'
import ComplexConversation from '../page/stories/conversations/multi_tool_multi_turn'
import * as Mojom from './mojom'
import { createConversationTurnWithDefaults } from './test_data_utils'
import {
  stringifyConversationData,
  parseConversationData,
  serializeConversationForSharing,
  type ConversationData,
} from './conversation_serialization'

const sampleAssociatedContent: Mojom.AssociatedContent = {
  uuid: 'assoc-1',
  contentType: Mojom.ContentType.PageContent,
  contentId: 1,
  title: 'Sample page',
  url: { url: 'https://example.com/1' },
  contentUsedPercentage: 100,
  conversationTurnUuid: '83f505d6-8fe6-448c-aabd-016ed1e2fa82',
  toolsAttached: false,
}

// The story fixture is stored in its serialized form, so revive it to get the
// mojom types a conversation actually holds.
const sampleSharedConversation: ConversationData = {
  messages: parseConversationData(JSON.stringify(ComplexConversation)).messages,
  associatedContent: [sampleAssociatedContent],
  title: 'sample title',
}

/**
 * Deterministic high-entropy bytes, standing in for the already-compressed
 * contents of an uploaded image or screenshot.
 */
function pseudoRandomBytes(length: number): number[] {
  return Array.from(
    { length },
    (_, i) => (Math.imul(i + 1, 2654435761) >>> 24) & 0xff,
  )
}

function conversationWithUploadedFile(data: number[]): ConversationData {
  return {
    messages: [
      createConversationTurnWithDefaults({
        text: 'What is this image?',
        characterType: Mojom.CharacterType.HUMAN,
        uploadedFiles: [
          {
            filename: 'lion.png',
            filesize: data.length,
            data,
            type: Mojom.UploadedFileType.kImage,
            extractedText: undefined,
          },
        ],
      }),
    ],
    associatedContent: [],
    title: 'conversation with an attachment',
  }
}

function getUploadedFileData(data: ConversationData): number[] | undefined {
  return data.messages[0].uploadedFiles?.[0].data
}

describe('conversation serialization', () => {
  it('should serialize and deserialize conversation data with bigint fields', () => {
    const serialized = stringifyConversationData(sampleSharedConversation)
    const deserialized = parseConversationData(serialized)
    expect(deserialized).toEqual(sampleSharedConversation)
  })

  it('provides a known shared conversation viewer export format', () => {
    const serialized = serializeConversationForSharing(sampleSharedConversation)
    const parsed = JSON.parse(serialized)

    expect(parsed).toHaveProperty('version')
    expect(parsed.version).toMatch(/^\d+\.\d+\.\d+$/)

    expect(parsed).toHaveProperty('data')
    const deserialized = parseConversationData(parsed.data)
    expect(deserialized).toEqual(sampleSharedConversation)
  })

  it('round-trips uploaded file bytes losslessly', () => {
    // Every byte value, so encoding is exercised across the full range,
    // including 0x00 and values which aren't printable ASCII.
    const allByteValues = Array.from({ length: 256 }, (_, i) => i)
    const conversation = conversationWithUploadedFile(allByteValues)

    const deserialized = parseConversationData(
      stringifyConversationData(conversation),
    )

    expect(deserialized).toEqual(conversation)
    // Mojom byte arrays are number[], which is what consumers of
    // UploadedFile.data expect - the bytes must not come back as a typed array.
    expect(Array.isArray(getUploadedFileData(deserialized))).toBe(true)
    expect(getUploadedFileData(deserialized)).toEqual(allByteValues)
  })

  it('encodes uploaded file bytes as base64 rather than JSON numbers', () => {
    const bytes = pseudoRandomBytes(4096)
    const serialized = stringifyConversationData(
      conversationWithUploadedFile(bytes),
    )

    // The bytes are a single tagged base64 string, not a list of decimals.
    expect(serialized).toContain('"$bytes":"')
    expect(serialized).not.toContain(bytes.slice(0, 8).join(','))

    // base64 costs a flat 1.33x plus the tag, where one decimal per byte costs
    // ~3.6x for high-entropy data. Allow generous headroom over 1.33x for the
    // rest of the conversation, but assert we're nowhere near the old size.
    expect(serialized.length).toBeLessThan(bytes.length * 1.5)
  })

  it('leaves short number arrays as numbers', () => {
    // Below the encoding threshold base64 would cost more than it saves, so
    // small byte arrays stay as JSON numbers - and must still round-trip.
    const bytes = pseudoRandomBytes(8)
    const conversation = conversationWithUploadedFile(bytes)
    const serialized = stringifyConversationData(conversation)

    expect(serialized).toContain(bytes.join(','))
    expect(serialized).not.toContain('"$bytes":"')
    expect(parseConversationData(serialized)).toEqual(conversation)
  })

  it('leaves long number arrays which are not bytes alone', () => {
    // The encoder recognises byte arrays by shape, so it must only claim
    // arrays it can actually represent as bytes.
    const notBytes = Array.from({ length: 128 }, (_, i) => i * 1000.5)
    const conversation = conversationWithUploadedFile(notBytes)
    const serialized = stringifyConversationData(conversation)

    expect(serialized).not.toContain('"$bytes":"')
    expect(parseConversationData(serialized)).toEqual(conversation)
  })
})
