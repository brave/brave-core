// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import {loadTimeData} from '//resources/js/load_time_data.js'
import {html, nothing} from '//resources/lit/v3_0/lit.rollup.js'

import {CustomProfileImageStrings as S} from './brave_generated_resources_webui_strings.js'
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
              aria-label="${loadTimeData.getString(
                S.CUSTOM_PROFILE_IMAGE_SELECTED_PREVIEW_LABEL,
              )}"
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
              title="${loadTimeData.getString(
                S.CUSTOM_PROFILE_IMAGE_UPLOAD_TOOLTIP,
              )}"
              @click="${this.onUploadClick_}"
            >
              <leo-icon
                id="plusIcon"
                name="plus-add"
                slot="icon-before"
                aria-hidden="true"
              ></leo-icon>
              <span id="previewLabel"
                >${loadTimeData.getString(
                  S.CUSTOM_PROFILE_IMAGE_UPLOAD_TOOLTIP,
                )}</span
              >
            </leo-button>
          `}

      <div id="title"
        >${loadTimeData.getString(
          S.CUSTOM_PROFILE_IMAGE_UPLOAD_ACTION,
        )}</div
      >

      <div id="actions">
        <leo-button
          id="uploadButton"
          class="action-button"
          kind="filled"
          size="small"
          title="${loadTimeData.getString(
            this.isSaved_()
              ? S.CUSTOM_PROFILE_IMAGE_REPLACE_TOOLTIP
              : S.CUSTOM_PROFILE_IMAGE_UPLOAD_TOOLTIP,
          )}"
          aria-label="${loadTimeData.getString(
            this.isSaved_()
              ? S.CUSTOM_PROFILE_IMAGE_REPLACE_TOOLTIP
              : S.CUSTOM_PROFILE_IMAGE_UPLOAD_TOOLTIP,
          )}"
          @click="${this.onUploadClick_}"
        >
          ${loadTimeData.getString(
            this.isSaved_()
              ? S.CUSTOM_PROFILE_IMAGE_REPLACE_ACTION
              : S.CUSTOM_PROFILE_IMAGE_UPLOAD_ACTION,
          )}
        </leo-button>
        ${this.isSaved_()
          ? html`
              <leo-button
                id="removeButton"
                kind="outline"
                size="small"
                title="${loadTimeData.getString(
                  S.CUSTOM_PROFILE_IMAGE_REMOVE_TOOLTIP,
                )}"
                aria-label="${loadTimeData.getString(
                  S.CUSTOM_PROFILE_IMAGE_REMOVE_TOOLTIP,
                )}"
                @click="${this.onRemoveClick_}"
              >
                ${loadTimeData.getString(
                  S.CUSTOM_PROFILE_IMAGE_REMOVE_ACTION,
                )}
              </leo-button>
            `
          : nothing}
      </div>

      ${this.hasValidationError_
        ? html`
            <div id="fileError" role="alert"
              >${loadTimeData.getString(
                S.CUSTOM_PROFILE_IMAGE_INVALID_IMAGE,
              )}</div
            >
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
