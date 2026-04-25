// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html, nothing} from
  '//resources/lit/v3_0/lit.rollup.js'

import type {LeoModelSelectorElement} from
  './model_selector.js'

export function getHtml(this: LeoModelSelectorElement) {
  return html`<!--_html_template_start_-->
    <leo-dropdown
      value="${this.selectedKey}"
      @change="${this.onSelectionChange_}"
    >
      <div slot="value">${this.selectedDisplayName}</div>
      <div class="menu-section-title">
        <span>$i18n{braveLeoModelSectionTitle}</span>
      </div>
      ${this.models.map((entry) => html`
        <leo-option value="${entry.model.key}">
          <div class="menu-item-with-icon">
            <div>
              <div>${entry.model.displayName}</div>
              ${entry.model.options.leoModelOptions
                  !== undefined ? html`
                <p class="model-subtitle">
                  ${entry.subtitle}
                </p>
              ` : html`
                <p class="model-subtitle">
                  ${entry.model.options
                      .customModelOptions
                      ?.modelRequestName}
                </p>
              `}
            </div>
            ${this.shouldShowPremiumLabel_(entry)
                ? html`
              <leo-label mode="outline" color="blue">
                $i18n{braveLeoPremiumLabelNonPremium}
              </leo-label>
            ` : nothing}
          </div>
        </leo-option>
      `)}
    </leo-dropdown>
  <!--_html_template_end_-->`
}
