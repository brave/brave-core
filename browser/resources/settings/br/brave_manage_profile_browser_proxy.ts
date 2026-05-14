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

// Wraps raw image bytes in a `mojo_base.mojom.BigBuffer`. The TS Mojo
// binding requires the union's `bytes` arm to be a plain number[]; for
// large payloads we therefore prefer the `sharedMemory` arm and let Mojo
// transfer the buffer handle to the browser without per-byte boxing. The
// browser-side handler reads either backing store transparently via
// `base::span(big_buffer)`.
function bytesToBigBuffer(bytes: Uint8Array): BigBuffer {
  if (bytes.byteLength < kInlineUploadThresholdBytes) {
    return { bytes: Array.from(bytes) } as BigBuffer
  }
  const sharedBuffer = Mojo.createSharedBuffer(bytes.byteLength)
  const mapping =
    new Uint8Array(sharedBuffer.handle.mapBuffer(0, bytes.byteLength).buffer)
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

  // Uploads new bytes as the user's custom avatar. `bytes` is the raw
  // contents of the user-selected image file (any common codec accepted by
  // the sandboxed image decoder is allowed). Multi-MiB uploads ride a
  // `BigBuffer` shared-memory handle to skip the per-byte boxing cost of an
  // `array<uint8>` Mojo argument.
  async setCustomAvatar(bytes: Uint8Array): Promise<SetCustomAvatarResult> {
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
