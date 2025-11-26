// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/no-unnecessary-type-assertion */

import { html, nothing } from 'chrome://resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveContentContainersElement } from './containers.js'
import { ContainersStrings } from '../brave_generated_resources_webui_strings.js'
import { Icon } from '../containers.mojom-webui.js'
import { skColorToHexColor } from 'chrome://resources/js/color_utils.js'
import backgroundColors from './background_colors.js'

// Bootstrap the custom element that are used in this component.
import './containers_icon.js'
import './containers_background_chip.js'

export function getHtml(this: SettingsBraveContentContainersElement) {
  return html`<!--_html_template_start_-->
    <settings-section id="containers" page-title="$i18n{SETTINGS_CONTAINERS_SECTION_LABEL}">
      <div class="cr-row first two-line">
        <div class="flex">
          <localized-link
            id="learn-more"
            .localizedString="${this.i18nAdvanced(
              ContainersStrings.SETTINGS_CONTAINERS_SECTION_DESCRIPTION,
            )}"
            .linkUrl="${this.i18n('containersLearnMoreURL')}"
          >
          </localized-link>
        </div>
        <cr-button
          @click="${this.onAddContainerClick_}"
          size="small"
        >
          $i18n{SETTINGS_CONTAINERS_ADD_CONTAINER_LABEL}
        </cr-button>
      </div>

      ${this.containersList_?.length
        ? html`
            <div class="cr-row continuation">
              <div class="list">
                ${this.containersList_.map(
                  (item) => html`
                    <div class="container">
                      <div class="icon-and-label">
                        <settings-brave-content-containers-icon
                          icon="${item.icon}"
                          background-color="${skColorToHexColor(
                            item.backgroundColor,
                          )}"
                          disabled
                        ></settings-brave-content-containers-icon>
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
                <div class="flex editing-container">
                  <section class="name-section">
                    <div class="edit-subsection-label">
                      $i18n{SETTINGS_CONTAINERS_CONTAINER_NAME_LABEL}
                    </div>
                    <cr-input
                      .value="${this.editingContainer_.name}"
                      @input="${this.onContainerNameInput_}"
                      autofocus
                      required
                      auto-validate
                      pattern="^.*\\S.*$"
                      placeholder="$i18n{SETTINGS_CONTAINERS_CONTAINER_NAME_PLACEHOLDER}"
                    ></cr-input>
                  </section>
                  <section class="background-colors-section">
                    <div class="edit-subsection-label">
                        $i18n{SETTINGS_CONTAINERS_CONTAINER_COLOR_LABEL}
                    </div>
                    <div class="background-colors-list">
                      ${backgroundColors.map(
                        (color) => html`
                          <settings-brave-content-containers-background-chip
                            ?selected="${color.value
                            === this.editingContainer_!.backgroundColor.value}"
                            background-color="${skColorToHexColor(color)}"
                            @background-selected="${this
                              .onContainersBackgroundColorSelected_}"
                          ></settings-brave-content-containers-background-chip>
                        `,
                      )}
                    </div>
                  </section>
                  <section class="icons-section">
                    <div class="edit-subsection-label">
                        $i18n{SETTINGS_CONTAINERS_CONTAINER_ICON_LABEL}
                    </div>
                    <div class="icons-list">
                      ${[...Array(Icon.MAX_VALUE - Icon.MIN_VALUE + 1).keys()]
                        .map((i) => Icon.MIN_VALUE + i)
                        .map(
                          (icon) => html`
                            <settings-brave-content-containers-icon
                              background-color="${skColorToHexColor(
                                this.editingContainer_!.backgroundColor,
                              )}"
                              icon=${icon}
                              ?selected="${icon === this.editingContainer_!.icon}"
                              @icon-selected="${this.onContainersIconSelected_}"
                            ></settings-brave-content-containers-icon>
                          `,
                        )}
                    </section>
                  </div>
                </div>
                ${this.editDialogError_
                    ? html`
                        <div class="error-message">${this.editDialogError_}</div>
                      `
                    : nothing}
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
                  ?disabled="${
                    !this.editingContainer_?.name || this.isEditDialogNameInvalid_
                  }"
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
            >
              <div slot="title">
                $i18n{SETTINGS_CONTAINERS_DELETE_CONTAINER_LABEL}
              </div>
              <div slot="body">
                <div class="label">
                  ${this.i18n(
                    ContainersStrings.SETTINGS_CONTAINERS_DELETE_CONTAINER_DESCRIPTION,
                    this.deletingContainer_.name,
                  )}
                </div>
                ${this.deleteDialogError_
                  ? html`
                      <div class="error-message">${this.deleteDialogError_}</div>
                    `
                  : nothing}
              </div>
              <div slot="button-container">
                <cr-button
                  class="cancel-button"
                  @click="${this.onCancelDialog_}"
                >
                  $i18n{cancel}
                </cr-button>
                <cr-button
                  class="tonal-button"
                  @click="${this.onDeleteContainerFromDialog_}"
                >
                  $i18n{delete}
                </cr-button>
              </div>
            </cr-dialog>
          `
        : nothing}
    </settings-section>
    <!--_html_template_end_-->`
}
