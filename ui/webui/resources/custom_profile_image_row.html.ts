// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_button/cr_button.js'
import '//resources/cr_elements/cr_icon/cr_icon.js'
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js'
import '//resources/cr_elements/icons.html.js'

import {html, nothing} from '//resources/lit/v3_0/lit.rollup.js'

import type {BrCustomProfileImageRowElement} from './custom_profile_image_row.js'

export function getHtml(this: BrCustomProfileImageRowElement) {
  return html`
    <section
      id="row"
      class="state-${this.state}
        ${this.shouldRenderTitle_() ? '' : 'title-hidden'}"
      aria-labelledby="${this.shouldRenderTitle_() ? 'title' : nothing}"
      aria-label="${this.shouldRenderTitle_() ? nothing : this.titleLabel}"
    >
      ${this.isSaved_()
        ? html`
            <div
              id="preview"
              class="selected"
              role="img"
              aria-label="${this.selectedPreviewLabel || this.previewLabel}"
            >
              ${this.localPreviewUrl_
                ? html`<img
                    id="previewImage"
                    alt=""
                    src="${this.localPreviewUrl_}"
                  />`
                : html`<cr-icon
                    id="plusIcon"
                    icon="cr:add"
                    aria-hidden="true"
                  ></cr-icon>`}
              <cr-icon
                id="selectedIndicator"
                icon="cr:check"
                aria-hidden="true"
              ></cr-icon>
            </div>
          `
        : html`
            <cr-icon-button
              id="preview"
              class="no-overlap"
              iron-icon="cr:add"
              aria-label="${this.uploadTooltip || this.previewLabel}"
              @click="${this.onUploadClick_}"
            ></cr-icon-button>
          `}

      ${this.shouldRenderTitle_()
        ? html`<div id="title">${this.titleLabel}</div>`
        : nothing}

      <div id="actions">
        <cr-button
          id="uploadButton"
          class="action-button"
          title="${this.getUploadButtonTooltip_() || nothing}"
          aria-label="${this.getUploadButtonTooltip_() || nothing}"
          @click="${this.onUploadClick_}"
        >
          ${this.isSaved_() ? this.replaceLabel : this.titleLabel}
        </cr-button>
        ${this.isSaved_()
          ? html`
              <cr-button
                id="removeButton"
                title="${this.removeTooltip || nothing}"
                aria-label="${this.removeTooltip || nothing}"
                @click="${this.onRemoveClick_}"
              >
                ${this.removeLabel}
              </cr-button>
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
