/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { getDeepActiveElement, hasKeyModifiers } from '//resources/js/util.js'

import { getCss } from './brave_account_otp_input.css.js'
import { getHtml } from './brave_account_otp_input.html.js'

const BASE32_CHAR_REGEX = /^[A-Z2-7]$/
const BASE32_SANITIZE_REGEX = /[^A-Z2-7]/g

type LeoInputElement = HTMLElement & {
  value: string
}

export type OtpInputEventDetail = { code: string; isCodeValid: boolean }

export class BraveAccountOtpInputElement extends CrLitElement {
  static get is() {
    return 'brave-account-otp-input'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      length: { type: Number },
    }
  }

  protected onPaste(e: ClipboardEvent) {
    e.preventDefault()

    const inputs = this.getInputs()
    const activeElement = getDeepActiveElement()
    const startIndex = Math.max(
      0,
      inputs.findIndex((input) => input.shadowRoot?.contains(activeElement)),
    )

    const chars = (e.clipboardData?.getData('text') || '')
      .toUpperCase()
      .replace(BASE32_SANITIZE_REGEX, '')
    for (const [offset, char] of [...chars].entries()) {
      const input = inputs[startIndex + offset]
      if (!input) {
        break
      }
      input.value = char
    }

    this.focusInput(Math.min(startIndex + chars.length, this.length - 1))
    this.emitCode()
  }

  // Handle direct text input by validating Base32 (A-Z, 2-7) and overwriting
  // the slot. Done in `beforeinput` to block invalid characters before they
  // enter the field and to bypass native insertion (caret-dependent), ensuring
  // consistent single-character behavior.
  protected onBeforeInput(e: InputEvent, index: number) {
    if (e.inputType !== 'insertText') {
      return
    }

    const char = e.data?.toUpperCase() ?? ''
    if (!BASE32_CHAR_REGEX.test(char)) {
      e.preventDefault()
      return
    }

    const input = this.getInput(index)
    if (!input) {
      return
    }

    e.preventDefault()
    input.value = char

    if (index < this.length - 1) {
      this.focusInput(index + 1)
    }

    this.emitCode()
  }

  // Collapse selection on focus (caret at end) to avoid the visual highlight.
  protected onFocus(detail: {
    innerEvent: Event & { target: HTMLInputElement }
  }) {
    const target = detail.innerEvent.target
    target.setSelectionRange(target.value.length, target.value.length)
  }

  // Fallback for native edits not handled in `beforeinput` (non-insertText
  // paths). Normal typing is handled in `beforeinput`, here we only normalize
  // the final value and emit the code.
  protected onInput(detail: { value: string }, index: number) {
    const input = this.getInput(index)
    if (!input) {
      return
    }

    input.value = detail.value
      .toUpperCase()
      .replace(BASE32_SANITIZE_REGEX, '')
      .slice(-1)

    this.emitCode()
  }

  // Handle navigation and editing keys explicitly.
  // Note: `Delete` is handled manually because the caret is always placed at
  // the end, so native delete would otherwise do nothing.
  protected onKeyDown(detail: { innerEvent: KeyboardEvent }, index: number) {
    const e = detail.innerEvent
    if (hasKeyModifiers(e)) {
      return
    }

    switch (e.key) {
      case 'ArrowLeft':
      case 'ArrowRight':
        return this.handleArrowKey(e, index)
      case 'Backspace':
        return this.handleBackspaceKey(e, index)
      case 'Delete':
        this.handleDeleteKey(e, index)
    }
  }

  private handleArrowKey(e: KeyboardEvent, index: number) {
    const nextIndex =
      e.key === 'ArrowLeft'
        ? index - 1
        : e.key === 'ArrowRight'
          ? index + 1
          : null

    if (nextIndex !== null && nextIndex >= 0 && nextIndex < this.length) {
      e.preventDefault()
      this.focusInput(nextIndex)
    }
  }

  private handleBackspaceKey(e: KeyboardEvent, index: number) {
    const input = this.getInput(index)
    if (!input || input.value) {
      // No-op: no field or let native backspace handle current value.
      return
    }

    const previousInput = this.getInput(index - 1)
    if (!previousInput) {
      return
    }

    e.preventDefault()
    previousInput.value = ''
    this.focusInput(index - 1)
    this.emitCode()
  }

  private handleDeleteKey(e: KeyboardEvent, index: number) {
    const input = this.getInput(index)
    if (!input || !input.value) {
      return
    }

    e.preventDefault()
    input.value = ''
    this.emitCode()
  }

  private getInputs(): LeoInputElement[] {
    return [...this.renderRoot.querySelectorAll<LeoInputElement>('leo-input')]
  }

  private getInput(index: number): LeoInputElement | undefined {
    return this.getInputs()[index]
  }

  private focusInput(index: number) {
    this.getInput(index)?.focus()
  }

  private getCode(): string {
    return this.getInputs()
      .map((input) => input.value)
      .join('')
  }

  private emitCode() {
    const code = this.getCode()
    this.fire('otp-input', {
      code,
      isCodeValid: code.length === this.length,
    } satisfies OtpInputEventDetail)
  }

  protected accessor length = 6
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-otp-input': BraveAccountOtpInputElement
  }
}

customElements.define(
  BraveAccountOtpInputElement.is,
  BraveAccountOtpInputElement,
)
