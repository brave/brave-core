// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html, nothing } from 'chrome://resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveContentContainersElement } from './containers.js'

export function getHtml(this: SettingsBraveContentContainersElement) {
  return html`<!--_html_template_start_-->
    <div class="cr-row first two-line">
      <div class="flex">
        <div class="label">$i18nRaw{containersDesc}</div>
      </div>
      <cr-button
        @click="${this.onAddContainerClick_}"
        size="small"
      >
        $i18n{containersAddContainer}
      </cr-button>
    </div>

    ${this.containersList_?.length
      ? html`
          <div class="cr-row continuation">
            <div class="list">
              ${this.containersList_.map(
                (item) => html`
                  <div class="container">
                    <div>
                      <div class="label">${item.name}</div>
                    </div>
                    <div>
                      <cr-icon-button
                        @click="${this.onEditContainerClick_}"
                        data-id="${item.id}"
                        class="size-20"
                        iron-icon="edit-pencil"
                      >
                      </cr-icon-button>
                      <cr-icon-button
                        @click="${this.onDeleteContainerClick_}"
                        data-id="${item.id}"
                        class="size-20"
                        iron-icon="trash"
                      >
                      </cr-icon-button>
                    </div>
                  </div>
                `,
              )}
            </div>
          </div>
        `
      : nothing}
    ${this.editingContainer_
      ? html`
          <cr-dialog
            id="editContainerDialog"
            show-close-button
            show-on-attach
            @close="${this.onCancelDialog_}"
          >
            <div slot="title">${this.getEditDialogTitle_()}</div>
            <div slot="body">
              <div class="flex">
                <cr-input
                  .value="${this.editingContainer_.name}"
                  @input="${this.onContainerNameInput_}"
                  autofocus
                  required
                  auto-validate
                  pattern=".+"
                  label="$i18n{containersContainerName}"
                  placeholder="$i18n{containersContainerNamePlaceholder}"
                >
                </cr-input>
              </div>
            </div>
            <div slot="button-container">
              <cr-button
                class="cancel-button"
                @click="${this.onCancelDialog_}"
              >
                $i18n{cancel}
              </cr-button>
              <cr-button
                class="action-button"
                @click="${this.onSaveContainerFromDialog_}"
                ?disabled="${!this.editingContainer_.name}"
              >
                $i18n{save}
              </cr-button>
            </div>
          </cr-dialog>
        `
      : nothing}
    ${this.deletingContainer_
      ? html`
          <cr-dialog
            id="deleteContainerDialog"
            show-close-button
            show-on-attach
            @close="${this.onCancelDialog_}"
            ?no-cancel="${this.isRemoving_}"
          >
            <div slot="title">$i18n{containersDeleteContainer}</div>
            <div slot="body">
              <div class="label">
                ${this.i18n(
                  'containersDeleteContainerDescription',
                  this.deletingContainer_.name,
                )}
              </div>
            </div>
            <div slot="button-container">
              <cr-button
                class="cancel-button"
                @click="${this.onCancelDialog_}"
                ?disabled="${this.isRemoving_}"
              >
                $i18n{cancel}
              </cr-button>
              <cr-button
                class="tonal-button"
                @click="${this.onDeleteContainerFromDialog_}"
                ?disabled="${this.isRemoving_}"
              >
                $i18n{delete}
              </cr-button>
            </div>
          </cr-dialog>
        `
      : nothing}
    <!--_html_template_end_-->`
}
