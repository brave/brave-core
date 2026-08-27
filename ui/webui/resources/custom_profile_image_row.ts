// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js'
import type {CSSResultGroup} from '//resources/lit/v3_0/lit.rollup.js'

import {getCss} from './custom_profile_image_row.css.js'
import {getHtml} from './custom_profile_image_row.html.js'

/** States supported by the custom profile image row. */
export type CustomProfileImageState = 'empty' | 'saved-active'

/** Shared custom-profile image control with a session-only local preview. */
export class BrCustomProfileImageRowElement extends CrLitElement {
  static get is() {
    return 'br-custom-profile-image-row'
  }

  static override get styles(): CSSResultGroup {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      hideTitle: {
        type: Boolean,
        attribute: 'hide-title',
        reflect: true,
      },
      hasValidationError_: {type: Boolean, state: true},
      localPreviewUrl_: {type: String, state: true},
    }
  }

  accessor hideTitle: boolean = false
  protected accessor hasValidationError_: boolean = false
  protected accessor localPreviewUrl_: string = ''

  private uploadAttemptId_: number = 0

  get state(): CustomProfileImageState {
    return this.getState_()
  }

  protected isSaved_(): boolean {
    return !!this.localPreviewUrl_
  }

  protected getState_(): CustomProfileImageState {
    return this.isSaved_() ? 'saved-active' : 'empty'
  }

  override disconnectedCallback() {
    this.clearLocalPreview_()
    super.disconnectedCallback()
  }

  protected onUploadClick_() {
    this.shadowRoot.querySelector<HTMLInputElement>('#fileInput')!.click()
  }

  protected onRemoveClick_() {
    this.clearLocalPreview_()
  }

  protected onFileChange_(event: Event) {
    const input = event.target as HTMLInputElement
    const file = input.files?.[0]
    input.value = ''
    if (file) {
      void this.replaceLocalPreview_(file)
    }
  }

  private clearLocalPreview_() {
    ++this.uploadAttemptId_
    this.hasValidationError_ = false
    this.revokeLocalPreviewUrl_()
  }

  private async replaceLocalPreview_(file: File) {
    const uploadAttemptId = ++this.uploadAttemptId_
    if (!file.type.toLowerCase().startsWith('image/')) {
      this.showValidationError_(uploadAttemptId)
      return
    }

    const previewUrl = URL.createObjectURL(file)
    const image = new Image()
    image.src = previewUrl
    try {
      await image.decode()
    } catch {
      URL.revokeObjectURL(previewUrl)
      this.showValidationError_(uploadAttemptId)
      return
    }

    if (!this.isConnected || uploadAttemptId !== this.uploadAttemptId_) {
      URL.revokeObjectURL(previewUrl)
      return
    }

    this.revokeLocalPreviewUrl_()
    this.localPreviewUrl_ = previewUrl
    this.hasValidationError_ = false
  }

  private showValidationError_(uploadAttemptId: number) {
    if (!this.isConnected || uploadAttemptId !== this.uploadAttemptId_) {
      return
    }
    this.hasValidationError_ = true
  }

  private revokeLocalPreviewUrl_() {
    if (!this.localPreviewUrl_) {
      return
    }
    URL.revokeObjectURL(this.localPreviewUrl_)
    this.localPreviewUrl_ = ''
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'br-custom-profile-image-row': BrCustomProfileImageRowElement
  }
}

customElements.define(
  BrCustomProfileImageRowElement.is,
  BrCustomProfileImageRowElement,
)
