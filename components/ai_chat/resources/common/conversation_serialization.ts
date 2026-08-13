// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '$web-common/loadTimeData'
import * as Mojom from './mojom'

// This type is free to change - the same version of this code will be used to
// serialize within the browser and deserialize within the shared conversation
// viewer.
export type ConversationData = {
  messages: Mojom.ConversationTurn[]
  // Favicons are provided by the client which serialized the conversation,
  // since the viewer has no access to the browser's favicon service. They are
  // optional because a favicon may not have been retrievable, and because
  // conversations shared before they were included don't have them.
  associatedContent: Mojom.AssociatedContentWithFavicon[]
  title: string
}

export interface SerializedConversation {
  /**
   * The version of the Brave client that produced this serialized conversation.
   */
  version: string

  /**
   * A JSON string of ConversationData, in which bigint and byte array fields
   * are represented as tagged objects (see the replacer below).
   */
  data: string
}

// Mojom conversation turns contain bigint fields (e.g. mojo.Time
// `internalValue`), which JSON cannot represent natively. We encode every
// bigint as a small tagged object so the value round-trips losslessly even
// when it exceeds Number.MAX_SAFE_INTEGER.
const BIGINT_TAG = '$bigint'

// Mojom byte arrays (`array<uint8>`, e.g. UploadedFile.data holding image,
// screenshot and PDF bytes) arrive as number[], which JSON writes as one
// decimal per byte plus a separator - about 3.6x the raw size for the
// high-entropy bytes of an already-compressed image ("137,80,78,71,"). base64
// is a flat 1.33x and its alphabet needs no JSON string escaping, so tagging
// and encoding byte arrays makes the attachments which dominate a shared
// conversation around 2.7x smaller.
const BYTES_TAG = '$bytes'

// Byte arrays shorter than this are left as numbers: the tag object and base64
// padding cost more than they save, and it keeps short number arrays which
// aren't really bytes out of the encoder.
const MIN_ENCODED_BYTE_LENGTH = 64

// btoa() takes a "binary string" of one character per byte, which we build with
// the variadic String.fromCharCode. Chunk the calls so a multi-megabyte
// attachment doesn't exceed the engine's argument count limit.
const FROM_CHAR_CODE_CHUNK_SIZE = 8192

/**
 * Identifies arrays which can be represented as bytes. This is a shape check
 * rather than a list of known mojom fields so that any future byte array is
 * encoded too. Encoding is symmetric with decoding, so a number array which
 * isn't really bytes but matches anyway still round-trips to an identical
 * value - the only cost of a false positive is a different wire
 * representation.
 */
function isByteArray(value: unknown): value is number[] {
  if (!Array.isArray(value) || value.length < MIN_ENCODED_BYTE_LENGTH) {
    return false
  }
  // Indexed access rather than every(), which skips holes in sparse arrays -
  // here a hole reads as undefined and disqualifies the array, so it keeps
  // being serialized as JSON's null.
  for (let i = 0; i < value.length; i++) {
    const byte = value[i]
    if (!Number.isInteger(byte) || byte < 0 || byte > 255) {
      return false
    }
  }
  return true
}

function bytesToBase64(bytes: number[]): string {
  let binary = ''
  for (let i = 0; i < bytes.length; i += FROM_CHAR_CODE_CHUNK_SIZE) {
    binary += String.fromCharCode(
      ...bytes.slice(i, i + FROM_CHAR_CODE_CHUNK_SIZE),
    )
  }
  return btoa(binary)
}

function base64ToBytes(base64: string): number[] {
  const binary = atob(base64)
  const bytes = new Array<number>(binary.length)
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i)
  }
  return bytes
}

function conversationReplacer(_key: string, value: unknown) {
  if (typeof value === 'bigint') {
    return { [BIGINT_TAG]: value.toString() }
  }
  if (isByteArray(value)) {
    return { [BYTES_TAG]: bytesToBase64(value) }
  }
  return value
}

function conversationReviver(_key: string, value: unknown) {
  if (value && typeof value === 'object' && Object.keys(value).length === 1) {
    const tagged = value as Record<string, unknown>
    if (typeof tagged[BIGINT_TAG] === 'string') {
      return BigInt(tagged[BIGINT_TAG])
    }
    if (typeof tagged[BYTES_TAG] === 'string') {
      return base64ToBytes(tagged[BYTES_TAG])
    }
  }
  return value
}

/** A bigint as written by conversationReplacer. */
type TaggedBigInt = { [BIGINT_TAG]: string }

/** A byte array as written by conversationReplacer. */
type TaggedBytes = { [BYTES_TAG]: string }

/**
 * The keys of T which JSON.stringify omits from its output, i.e. those whose
 * value can be undefined.
 */
type UndefinedKeys<T> = {
  [K in keyof T]-?: undefined extends T[K] ? K : never
}[keyof T]

/**
 * An object as parsed back from JSON. A property which was undefined was never
 * written, so it comes back missing rather than present-and-undefined. It can
 * also come back as null: mojom's nullable fields are typed as undefined but
 * the JS bindings decode them from the browser as null, which JSON keeps.
 */
type JsonifiedObject<T> = {
  [K in Exclude<keyof T, UndefinedKeys<T>>]: Jsonified<T[K]>
} & {
  [K in UndefinedKeys<T>]?: Jsonified<Exclude<T[K], undefined>> | null
}

/**
 * A value as parsed back from JSON without conversationReviver, so bigints and
 * encoded byte arrays are still in their tagged form. Byte arrays are a union
 * because only the arrays isByteArray accepts are encoded - shorter ones stay
 * as numbers.
 */
type Jsonified<T> = T extends bigint
  ? TaggedBigInt
  : T extends number[]
    ? number[] | TaggedBytes
    : T extends readonly (infer E)[]
      ? Array<Jsonified<E>>
      : T extends object
        ? JsonifiedObject<T>
        : T

/**
 * The shape of stringifyConversationData's output once JSON.parse'd without
 * conversationReviver - what a conversation looks like when it is kept as a
 * .json file rather than restored with parseConversationData.
 */
export type ConversationDataJson = Jsonified<ConversationData>

/**
 * Serializes all conversation data to a JSON string, which can be restored
 * with parseConversationData. This is a lossless serialization.
 */
export function stringifyConversationData(data: ConversationData): string {
  return JSON.stringify(data, conversationReplacer)
}

/**
 * Parses a JSON string produced by stringifyConversationData and restores
 * the original conversation data, including bigint and byte array fields.
 */
export function parseConversationData(dataJson: string): ConversationData {
  return JSON.parse(dataJson, conversationReviver) as ConversationData
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
    // The Brave client version that produced this payload, so the viewer can
    // choose the correct parsing algorithm for the data below.
    version: loadTimeData.getString('braveVersion'),
    data: stringifyConversationData(data),
  }

  return JSON.stringify(payload)
}
