// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import {html, nothing} from '//resources/lit/v3_0/lit.rollup.js'

import type {BrCustomProfileImageRowElement} from './custom_profile_image_row.js'

export function getHtml(this: BrCustomProfileImageRowElement) {
  return html`
    <section
      id="row"
      class="state-${this.getState_()}"
      aria-labelledby="title"
    >
      ${this.isSaved_()
        ? html`
            <div
              id="preview"
              class="selected"
              role="img"
              aria-label="${this.selectedPreviewLabel}"
            >
              <img
                id="previewImage"
                alt=""
                src="${this.localPreviewUrl_}"
              />
              <leo-icon
                id="selectedIndicator"
                name="check-normal"
                aria-hidden="true"
              ></leo-icon>
            </div>
          `
        : html`
            <leo-button
              id="preview"
              kind="plain-faint"
              size="jumbo"
              fab
              title="${this.uploadTooltip}"
              @click="${this.onUploadClick_}"
            >
              <leo-icon
                id="plusIcon"
                name="plus-add"
                slot="icon-before"
                aria-hidden="true"
              ></leo-icon>
              <span id="previewLabel">${this.uploadTooltip}</span>
            </leo-button>
          `}

      <div id="title">${this.titleLabel}</div>

      <div id="actions">
        <leo-button
          id="uploadButton"
          class="action-button"
          kind="filled"
          size="small"
          title="${this.getUploadButtonTooltip_()}"
          aria-label="${this.getUploadButtonTooltip_()}"
          @click="${this.onUploadClick_}"
        >
          ${this.isSaved_() ? this.replaceLabel : this.titleLabel}
        </leo-button>
        ${this.isSaved_()
          ? html`
              <leo-button
                id="removeButton"
                kind="outline"
                size="small"
                title="${this.removeTooltip}"
                aria-label="${this.removeTooltip}"
                @click="${this.onRemoveClick_}"
              >
                ${this.removeLabel}
              </leo-button>
            `
          : nothing}
      </div>

      ${this.hasValidationError_
        ? html`
            <div id="fileError" role="alert">${this.invalidImageLabel}</div>
          `
        : nothing}

      <input
        id="fileInput"
        type="file"
        accept="image/*"
        hidden
        @change="${this.onFileChange_}"
      />
    </section>
  `
}
