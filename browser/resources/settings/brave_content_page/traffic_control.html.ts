// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_components/localized_link/localized_link.js'

import { html, nothing } from 'chrome://resources/lit/v3_0/lit.rollup.js'

import { TrafficControlStrings } from '../brave_generated_resources_webui_strings.js'
import { SettingsBraveContentTrafficControlElement } from './traffic_control.js'

// Bootstrap the custom element used to render container destinations.
import './containers_icon.js'

export function getHtml(this: SettingsBraveContentTrafficControlElement) {
  return html`<!--_html_template_start_-->
    <settings-section
      id="traffic-control"
      page-title="$i18n{SETTINGS_TRAFFIC_CONTROL_SECTION_LABEL}"
    >
      <div class="cr-row first two-line">
        <div class="flex">
          <localized-link
            id="learn-more"
            .localizedString="${this.i18nAdvanced(
              TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_SECTION_DESCRIPTION,
            )}"
            .linkUrl="${this.i18n('trafficControlLearnMoreURL')}"
          >
          </localized-link>
        </div>
      </div>

      <settings-toggle-button
        pref-key="brave.traffic_control.enabled"
        icon="traffic-control"
        label="$i18n{SETTINGS_TRAFFIC_CONTROL_ENABLED_LABEL}"
      ></settings-toggle-button>

      ${this.enabledPref_?.value
        ? html`
            ${this.rulesList_?.length
              ? html`
                  <div class="cr-row continuation">
                    <div class="list">
                      ${this.rulesList_.map(
                        (item) => html`
                          <div class="rule">
                            <div class="rule-summary">
                              <div class="filter-source">
                                <site-favicon
                                  .url="${item.urlFilter}"
                                ></site-favicon>
                                <div class="url-filter">${item.urlFilter}</div>
                              </div>
                              <div class="container-target">
                                ${this.hasContainer_(item.target.containerId)
                                  ? html`
                                      <settings-brave-content-containers-icon
                                        icon="${this.containerIcon_(
                                          item.target.containerId,
                                        )}"
                                        background-color="${this.containerBackgroundColor_(
                                          item.target.containerId,
                                        )}"
                                      ></settings-brave-content-containers-icon>
                                      <div class="label">
                                        ${this.containerName_(
                                          item.target.containerId,
                                        )}
                                      </div>
                                    `
                                  : html`
                                      <div class="label">
                                        ${this.containerName_(
                                          item.target.containerId,
                                        )}
                                      </div>
                                    `}
                              </div>
                            </div>
                            <div>
                              <cr-icon-button
                                @click="${this.onEditRuleClick_}"
                                data-id="${item.id}"
                                class="size-20"
                                iron-icon="edit-pencil"
                              >
                              </cr-icon-button>
                              <cr-icon-button
                                @click="${this.onDeleteRuleClick_}"
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
            <div class="cr-row continuation rules-add-row">
              <cr-button
                @click="${this.onAddRuleClick_}"
                size="small"
              >
                $i18n{SETTINGS_TRAFFIC_CONTROL_ADD_RULE_LABEL}
              </cr-button>
            </div>
          `
        : nothing}
      ${this.editingRule_
        ? html`
            <cr-dialog
              show-close-button
              show-on-attach
              @close="${this.onEditDialogClose_}"
            >
              <div slot="title">
                ${this.editingRule_.id
                  ? html`$i18n{SETTINGS_TRAFFIC_CONTROL_EDIT_RULE_LABEL}`
                  : html`$i18n{SETTINGS_TRAFFIC_CONTROL_NEW_RULE_DIALOG_TITLE}`}
              </div>
              <div slot="body">
                <div class="flex editing-rule">
                  <section class="field-section enabled-row">
                    <div class="edit-subsection-label">
                      $i18n{SETTINGS_TRAFFIC_CONTROL_RULE_ENABLED_LABEL}
                    </div>
                    <cr-toggle
                      ?checked="${this.editingRule_.enabled}"
                      @change="${this.onRuleEnabledChange_}"
                    ></cr-toggle>
                  </section>
                  <div class="field-description">
                    $i18n{SETTINGS_TRAFFIC_CONTROL_RULE_DIALOG_DESCRIPTION}
                  </div>
                  <section class="field-section">
                    <div class="edit-subsection-label">
                      $i18n{SETTINGS_TRAFFIC_CONTROL_URL_FILTER_LABEL}
                    </div>
                    <cr-input
                      .value="${this.editingRule_.urlFilter}"
                      placeholder="$i18n{SETTINGS_TRAFFIC_CONTROL_URL_FILTER_PLACEHOLDER}"
                      @value-changed="${this.onUrlFilterValueChanged_}"
                      autofocus
                    ></cr-input>
                    <div class="field-hint">
                      $i18n{SETTINGS_TRAFFIC_CONTROL_URL_FILTER_HINT}
                    </div>
                  </section>
                  <section class="field-section">
                    <div class="edit-subsection-label">
                      $i18n{SETTINGS_TRAFFIC_CONTROL_CONTAINER_LABEL}
                    </div>
                    <settings-dropdown-menu
                      .value="${this.editingRule_.target.containerId ?? ''}"
                      .menuOptions="${this.containerMenuOptions_}"
                      label="$i18n{SETTINGS_TRAFFIC_CONTROL_CONTAINER_LABEL}"
                      @settings-control-change="${this
                        .onContainerSettingsControlChange_}"
                    ></settings-dropdown-menu>
                  </section>
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
                  @click="${this.onEditDialogCancelClick_}"
                >
                  $i18n{cancel}
                </cr-button>
                <cr-button
                  class="action-button"
                  ?disabled="${!this.canSaveEditingRule_()}"
                  @click="${this.onEditDialogSaveClick_}"
                >
                  ${this.editingRule_.id
                    ? html`$i18n{SETTINGS_TRAFFIC_CONTROL_SAVE_CHANGES_LABEL}`
                    : html`$i18n{SETTINGS_TRAFFIC_CONTROL_CREATE_RULE_LABEL}`}
                </cr-button>
              </div>
            </cr-dialog>
          `
        : nothing}
      ${this.deletingRule_
        ? html`
            <cr-dialog
              show-close-button
              show-on-attach
              @close="${this.onDeleteDialogClose_}"
            >
              <div slot="title">
                $i18n{SETTINGS_TRAFFIC_CONTROL_DELETE_RULE_LABEL}
              </div>
              <div slot="body">
                <div class="label">
                  $i18n{SETTINGS_TRAFFIC_CONTROL_DELETE_RULE_DESCRIPTION}
                </div>
                ${this.deleteDialogError_
                  ? html`
                      <div class="error-message">
                        ${this.deleteDialogError_}
                      </div>
                    `
                  : nothing}
              </div>
              <div slot="button-container">
                <cr-button
                  class="cancel-button"
                  @click="${this.onDeleteDialogCancelClick_}"
                >
                  $i18n{cancel}
                </cr-button>
                <cr-button
                  class="tonal-button"
                  @click="${this.onDeleteDialogConfirmClick_}"
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
