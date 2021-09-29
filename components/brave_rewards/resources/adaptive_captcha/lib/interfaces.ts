/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type CaptchaStatus =
  'pending' |
  'success' |
  'max-attempts-exceeded'

export type CaptchaResult = 'success' | 'failure' | 'error'

export interface HostState {
  loading: boolean
  captchaURL: string
  captchaStatus: CaptchaStatus
}

export type HostListener = (state: HostState) => void

export interface Host {
  state: HostState
  getString: (key: string) => string
  addListener: (callback: HostListener) => () => void
  handleCaptchaResult: (result: CaptchaResult) => void
}
