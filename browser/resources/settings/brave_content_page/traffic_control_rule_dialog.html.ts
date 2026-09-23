// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html, nothing } from 'chrome://resources/lit/v3_0/lit.rollup.js'

import { TrafficControlStrings } from '../brave_generated_resources_webui_strings.js'
import { SettingsBraveContentTrafficControlRuleDialogElement } from './traffic_control_rule_dialog.js'
import { urlFilterOf } from './traffic_control_utils.js'

export function getHtml(
  this: SettingsBraveContentTrafficControlRuleDialogElement,
) {
  return html`<!--_html_template_start_-->
    <cr-dialog
      show-close-button
      show-on-attach
      @close="${this.onDialogClose_}"
      @keydown="${this.onKeydown_}"
    >
      <div slot="title">
        ${this.draftRule_.id
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
              ?checked="${this.draftRule_.enabled}"
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
            <cr-textarea
              .value="${urlFilterOf(this.draftRule_)}"
              placeholder="$i18n{SETTINGS_TRAFFIC_CONTROL_URL_FILTER_PLACEHOLDER}"
              rows="3"
              autogrow
              autofocus
              spellcheck="false"
              @value-changed="${this.onUrlFilterValueChanged_}"
            ></cr-textarea>
            <div
              class="field-hint"
              .innerHTML="${this.i18nAdvanced(
                TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_URL_FILTER_HINT,
                { attrs: ['class'] },
              )}"
            ></div>
          </section>
          <section class="field-section">
            <div class="edit-subsection-label">
              $i18n{SETTINGS_TRAFFIC_CONTROL_CONTAINER_LABEL}
            </div>
            <settings-dropdown-menu
              .value="${this.containerDropdownValue_()}"
              .menuOptions="${this.containerMenuOptions_()}"
              label="$i18n{SETTINGS_TRAFFIC_CONTROL_CONTAINER_LABEL}"
              @settings-control-change="${this
                .onContainerSettingsControlChange_}"
            ></settings-dropdown-menu>
          </section>
        </div>
        ${this.error_
          ? html` <div class="error-message">${this.error_}</div> `
          : nothing}
      </div>
      <div slot="button-container">
        <cr-button
          class="cancel-button"
          @click="${this.onCancelClick_}"
        >
          $i18n{cancel}
        </cr-button>
        <cr-button
          class="action-button"
          ?disabled="${!this.canSave_()}"
          @click="${this.onSaveClick_}"
        >
          ${this.draftRule_.id
            ? html`$i18n{SETTINGS_TRAFFIC_CONTROL_SAVE_CHANGES_LABEL}`
            : html`$i18n{SETTINGS_TRAFFIC_CONTROL_CREATE_RULE_LABEL}`}
        </cr-button>
      </div>
    </cr-dialog>
    <!--_html_template_end_-->`
}
