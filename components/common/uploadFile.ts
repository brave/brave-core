// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export interface PickFilesOptions {
  /** When true, the user may select more than one file. Default false. */
  multiple?: boolean
  /**
   * Comma-separated accept list (HTML `accept` attribute), e.g. `"image/*"`,
   * `"application/pdf"`, or `".pdf,.docx"`.
   */
  accept?: string
  /**
   * Passed through to the input `capture` attribute when using file inputs for
   * camera/microphone where supported (typically mobile).
   */
  capture?: string
}

/**
 * Opens the native file picker and resolves with the chosen file(s).
 * Resolves with an empty array if the dialog is dismissed without a selection.
 */
export function pickFiles(options?: PickFilesOptions): Promise<File[]> {
  return new Promise((resolve) => {
    const input = document.createElement('input')
    input.type = 'file'
    input.multiple = options?.multiple ?? false
    if (options?.accept) {
      input.accept = options.accept
    }
    if (options?.capture) {
      input.setAttribute('capture', options.capture)
    }

    let settled = false
    const settle = (files: File[]) => {
      if (settled) {
        return
      }
      settled = true
      cleanup()
      resolve(files)
    }

    const cleanup = () => {
      input.removeEventListener('change', onChange)
      input.removeEventListener('cancel', onCancel)
      input.remove()
    }

    const onChange = () => {
      settle(Array.from(input.files ?? []))
    }

    const onCancel = () => {
      settle([])
    }

    input.addEventListener('change', onChange)
    input.addEventListener('cancel', onCancel)

    document.body.appendChild(input)
    input.click()
  })
}
