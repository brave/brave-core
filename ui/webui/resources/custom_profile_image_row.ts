// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js'
import type {
  CSSResultGroup,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

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
      state: {type: String, reflect: true},
      hideTitle: {type: Boolean, attribute: 'hide-title'},
      invalidImageLabel: {type: String, attribute: 'invalid-image-label'},
      replaceLabel: {type: String, attribute: 'replace-label'},
      replaceTooltip: {type: String, attribute: 'replace-tooltip'},
      removeLabel: {type: String, attribute: 'remove-label'},
      removeTooltip: {type: String, attribute: 'remove-tooltip'},
      selectedPreviewLabel: {
        type: String,
        attribute: 'selected-preview-label',
      },
      titleLabel: {type: String, attribute: 'title-label'},
      uploadTooltip: {type: String, attribute: 'upload-tooltip'},
      hasValidationError_: {type: Boolean, state: true},
      localPreviewUrl_: {type: String, state: true},
    }
  }

  accessor state: CustomProfileImageState = 'empty'
  accessor hideTitle: boolean = false
  accessor invalidImageLabel: string = ''
  accessor replaceLabel: string = ''
  accessor replaceTooltip: string = ''
  accessor removeLabel: string = ''
  accessor removeTooltip: string = ''
  accessor selectedPreviewLabel: string = ''
  accessor titleLabel: string = ''
  accessor uploadTooltip: string = ''
  protected accessor hasValidationError_: boolean = false
  protected accessor localPreviewUrl_: string = ''

  private uploadAttemptId_: number = 0

  protected isSaved_(): boolean {
    return this.state === 'saved-active'
  }

  protected shouldRenderTitle_(): boolean {
    return !this.hideTitle
  }

  protected getUploadButtonTooltip_(): string {
    return this.isSaved_() ? this.replaceTooltip : this.uploadTooltip
  }

  override disconnectedCallback() {
    this.clearLocalPreview_()
    super.disconnectedCallback()
  }

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)

    if (this.state !== 'empty' && this.state !== 'saved-active') {
      throw new Error(`Unsupported custom profile image state: ${this.state}`)
    }
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
    this.state = 'empty'
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
    this.state = 'saved-active'
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
