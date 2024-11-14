// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterStyleOverride(
    'safety-check',
    html`
    <style include="settings-shared">
      slot {
        box-shadow: none !important;
        background-color: transparent !important;
        border-radius: 0 !important;
        display: block;
        padding-bottom: 0 !important;
    }
    </style>
    `
)
RegisterPolymerTemplateModifications({
    'settings-safety-check-page': (templateContent) => {
        const safetyCheckParentLabel = templateContent.querySelector('.flex,.cr-padded-text')
        if (!safetyCheckParentLabel) {
            console.error('[Brave Settings Overrides] Could not find safetyCheckParentLabel on safety page.')
        } else {
            safetyCheckParentLabel.style.cssText = 'padding-right: 32px;'
        }
    }
})
