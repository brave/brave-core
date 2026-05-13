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

import {
  BraveManageProfileSettingsHandler,
  BraveManageProfileSettingsHandlerRemote,
  BraveManageProfileSettingsUICallbackRouter,
  CustomAvatarState,
  SetCustomAvatarError,
} from '../brave_manage_profile.mojom-webui.js'

export { SetCustomAvatarError }
export type { CustomAvatarState }

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

  // Uploads new bytes as the user's custom avatar. `base64Bytes` is the
  // base64-encoded contents of the user-selected image file (any common
  // codec accepted by the sandboxed image decoder is allowed).
  async setCustomAvatar(base64Bytes: string): Promise<SetCustomAvatarResult> {
    const { error, state } = await this.handler.setCustomAvatar(base64Bytes)
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
