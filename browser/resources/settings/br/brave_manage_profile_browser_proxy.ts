/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * Thin browser proxy for the Brave-specific "Upload your own image" row on
 * `brave://settings/manageProfile`. Wraps the three `chrome.send` /
 * `sendWithPromise` messages exposed by `BraveManageProfileHandler` and the
 * `brave-custom-avatar-changed` listener it fires.
 */

import { sendWithPromise } from 'chrome://resources/js/cr.js'

export interface BraveCustomAvatarState {
  // True when a user-uploaded custom profile avatar is currently persisted.
  hasAvatar: boolean
  // PNG data URL of the current custom avatar (omitted while it is still
  // loading from disk after a restart, or when `hasAvatar` is false).
  dataUrl?: string
}

export interface BraveManageProfileBrowserProxy {
  // Returns the current custom-avatar state (presence + preview data URL).
  getProfileCustomAvatar(): Promise<BraveCustomAvatarState>

  // Uploads new bytes as the user's custom avatar. `base64Bytes` is the
  // base64-encoded contents of the user-selected image file (any common
  // codec accepted by the sandboxed image decoder is allowed). Resolves
  // with the new state on success, rejects with a short error tag on
  // failure (e.g. "decode-failed", "too-large").
  setProfileCustomAvatar(base64Bytes: string): Promise<BraveCustomAvatarState>

  // Clears the user-uploaded custom avatar (also deletes the on-disk file).
  removeProfileCustomAvatar(): void
}

export class BraveManageProfileBrowserProxyImpl
  implements BraveManageProfileBrowserProxy {
  getProfileCustomAvatar() {
    return sendWithPromise<BraveCustomAvatarState>('getProfileCustomAvatar')
  }

  setProfileCustomAvatar(base64Bytes: string) {
    return sendWithPromise<BraveCustomAvatarState>(
      'setProfileCustomAvatar', base64Bytes)
  }

  removeProfileCustomAvatar() {
    chrome.send('removeProfileCustomAvatar')
  }

  static getInstance(): BraveManageProfileBrowserProxy {
    return instance || (instance = new BraveManageProfileBrowserProxyImpl())
  }

  static setInstance(obj: BraveManageProfileBrowserProxy) {
    instance = obj
  }
}

let instance: BraveManageProfileBrowserProxy | null = null
