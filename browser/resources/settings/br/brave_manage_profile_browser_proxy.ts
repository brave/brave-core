/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * Thin browser proxy for the Brave-specific "Upload your own image" row on
 * `brave://settings/manageProfile`. Wraps the Mojo
 * `BraveManageProfileSettingsHandler` interface and exposes a typed Promise
 * surface to the Polymer overlay in `settings_manage_profile.ts`.
 */

import type { BigBuffer } from
  'chrome://resources/mojo/mojo/public/mojom/base/big_buffer.mojom-webui.js'
import {
  BraveManageProfileSettingsHandler,
  BraveManageProfileSettingsHandlerRemote,
  BraveManageProfileSettingsUICallbackRouter,
  CustomAvatarState,
  SetCustomAvatarError,
} from '../brave_manage_profile.mojom-webui.js'

export { SetCustomAvatarError }
export type { CustomAvatarState }

// Threshold (bytes) above which we ship the upload via a shared-memory
// `BigBuffer` handle instead of inlining a `number[]` into the Mojo
// message. Below this, the boxing cost of `Array.from(bytes)` is bounded
// (~64K boxed Numbers); above it, a multi-MiB profile image would otherwise
// allocate millions of boxed Numbers and stall GC.
const kInlineUploadThresholdBytes = 64 * 1024

// Renderer-side cap on the upload payload size. Kept in sync with the
// browser-side `BraveManageProfileHandler::kMaxUploadBytes` (10 MiB) so the
// renderer can reject oversized files before reading them into memory or
// allocating a shared-memory region for the Mojo hop.
//
// The browser-side handler enforces the same limit again before decode; that
// check is the authoritative security boundary (a compromised renderer can
// skip this gate and call Mojo directly). `kMaxDecodedDimension` is enforced
// only in the browser after out-of-process decode.
export const kMaxUploadBytes = 10 * 1024 * 1024

// `MOJO_RESULT_OK` (0) from `MojoResult` in `third_party/blink/.../mojo.idl`.
// `Mojo.createSharedBuffer` / `MojoHandle.mapBuffer` return a result enum
// alongside the handle / ArrayBuffer; any non-zero value means the operation
// failed and the payload field is unset.
const kMojoResultOk = 0

// Thrown by `bytesToBigBuffer` when allocating or mapping the shared-memory
// region for an oversized upload fails (e.g., low memory or an OS shared-
// memory handle limit). Surfaced to the caller so the UI can present a
// clean error instead of corrupting the upload with a zero-length mapping.
export class CustomAvatarSharedBufferError extends Error {
  constructor(message: string) {
    super(message)
    this.name = 'CustomAvatarSharedBufferError'
  }
}

// Wraps raw image bytes in a `mojo_base.mojom.BigBuffer`. The TS Mojo
// binding requires the union's `bytes` arm to be a plain number[]; for
// large payloads we therefore prefer the `sharedMemory` arm and let Mojo
// transfer the buffer handle to the browser without per-byte boxing. The
// browser-side handler reads either backing store transparently via
// `base::span(big_buffer)`.
//
// `Mojo.createSharedBuffer` and `MojoHandle.mapBuffer` can fail (e.g., low
// memory, OS shared-memory handle limit). Both return a `{ result, … }`
// dictionary with `result === 0` (`MOJO_RESULT_OK`) on success; on failure
// the handle / ArrayBuffer field is unset. Validate each step explicitly
// and throw `CustomAvatarSharedBufferError` rather than passing a missing
// handle or null buffer through, which would otherwise corrupt the upload
// with a zero-length mapping or silently throw inside `new Uint8Array(…)`.
function bytesToBigBuffer(bytes: Uint8Array): BigBuffer {
  if (bytes.byteLength < kInlineUploadThresholdBytes) {
    return { bytes: Array.from(bytes) } as BigBuffer
  }
  const sharedBuffer = Mojo.createSharedBuffer(bytes.byteLength)
  if (sharedBuffer.result !== kMojoResultOk || !sharedBuffer.handle) {
    throw new CustomAvatarSharedBufferError(
      'Mojo.createSharedBuffer failed with result=' + sharedBuffer.result)
  }
  const mapResult = sharedBuffer.handle.mapBuffer(0, bytes.byteLength)
  if (mapResult.result !== kMojoResultOk || !mapResult.buffer) {
    throw new CustomAvatarSharedBufferError(
      'MojoHandle.mapBuffer failed with result=' + mapResult.result)
  }
  const mapping = new Uint8Array(mapResult.buffer)
  mapping.set(bytes)
  return {
    sharedMemory: {
      bufferHandle: sharedBuffer.handle,
      size: bytes.byteLength,
    },
  } as BigBuffer
}

// Result returned by `setCustomAvatar`. On success `error` is undefined and
// `state` reflects the freshly-saved avatar. On failure `error` is set and
// `state` is the unchanged current state.
export interface SetCustomAvatarResult {
  error?: SetCustomAvatarError
  state: CustomAvatarState
}

let instance: BraveManageProfileBrowserProxy | null = null

export class BraveManageProfileBrowserProxy {
  handler: BraveManageProfileSettingsHandlerRemote
  callbackRouter: BraveManageProfileSettingsUICallbackRouter

  private constructor(
    handler: BraveManageProfileSettingsHandlerRemote,
    callbackRouter: BraveManageProfileSettingsUICallbackRouter,
  ) {
    this.handler = handler
    this.callbackRouter = callbackRouter
  }

  // Returns the current custom-avatar state (presence + preview data URL).
  async getCustomAvatar(): Promise<CustomAvatarState> {
    const { state } = await this.handler.getCustomAvatar()
    return state
  }

  // Uploads new bytes as the user's custom avatar. `bytes` is the raw contents
  // of the user-selected image file. Payloads at or above
  // `kInlineUploadThresholdBytes` are wrapped by `bytesToBigBuffer` into a
  // `BigBuffer` shared-memory handle (with explicit `createSharedBuffer` /
  // `mapBuffer` validation); smaller payloads use the inline-bytes arm. The
  // browser reads either backing store via `base::span(big_buffer)`.
  //
  // Payloads larger than `kMaxUploadBytes` are rejected synchronously here
  // (mirroring the browser-side limit) so we don't allocate a shared-memory
  // region for, or copy, bytes that the handler would only reject again.
  // Callers should also gate on `file.size` before reading the bytes off
  // disk to avoid materializing oversized uploads in the first place.
  async setCustomAvatar(bytes: Uint8Array): Promise<SetCustomAvatarResult> {
    if (bytes.byteLength > kMaxUploadBytes) {
      const { state } = await this.handler.getCustomAvatar()
      return { error: SetCustomAvatarError.kTooLarge, state }
    }
    const { error, state } =
      await this.handler.setCustomAvatar(bytesToBigBuffer(bytes))
    return { error: error ?? undefined, state }
  }

  // Clears the user-uploaded custom avatar (also deletes the on-disk file).
  removeCustomAvatar(): void {
    this.handler.removeCustomAvatar()
  }

  // Uses the saved custom avatar again after the user had chosen a preset.
  activateCustomAvatar(): void {
    this.handler.activateCustomAvatar()
  }

  static getInstance(): BraveManageProfileBrowserProxy {
    if (!instance) {
      const handler = BraveManageProfileSettingsHandler.getRemote()
      const callbackRouter = new BraveManageProfileSettingsUICallbackRouter()
      handler.bindUI(callbackRouter.$.bindNewPipeAndPassRemote())
      instance = new BraveManageProfileBrowserProxy(handler, callbackRouter)
    }
    return instance
  }

  static setInstance(obj: BraveManageProfileBrowserProxy): void {
    instance = obj
  }
}
