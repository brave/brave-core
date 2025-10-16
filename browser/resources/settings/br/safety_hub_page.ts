/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, RegisterPolymerTemplateModifications } from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-safety-hub-page': (templateContent) => {
    // For some reason RegisterStyleOverride doesn't work with this template.
    templateContent.prepend(html`<style>
      #emptyStateModule {
        --iron-icon-fill-color: var(--google-green-700);
      }`)

    const safetyHubPasswordsCard = templateContent.getElementById('passwords')
    if (!safetyHubPasswordsCard) {
      console.error('[Settings] missing SafetyHub passwords card')
    } else {
      safetyHubPasswordsCard.setAttribute('hidden', 'true')
    }

    // Note: The #emptyStateModule lives inside a dom-if, so we need to select
    // that template first.
    const noRecommendationsHandler = templateContent.querySelector('[if="[[showNoRecommendationsState_]]"]')
    if (!noRecommendationsHandler) {
      console.error('[Settings]: missing showNoRecommendationsState_ dom-if')
    } else {
      const emptyStateModule = noRecommendationsHandler.content.getElementById('emptyStateModule')
      if (!emptyStateModule) {
        console.error('[Settings]: missing SafetyHubPage emptyStateModule')
      } else {
        emptyStateModule.setAttribute('header-icon', 'shield-done-filled')
      }
    }
  }
})
