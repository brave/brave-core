// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangleHtml } from '//resources/brave/lit_overriding.js'
import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js';

import { getHtml as getHtmlChromium } from './detail_view-chromium.html.js'
import type { ExtensionsDetailViewElement } from './detail_view.js';

export function getHtml(this: ExtensionsDetailViewElement) {
    return mangleHtml(getHtmlChromium, this, (root) => {
        const optionsSection = root.querySelector('#options-section')
        if (!optionsSection) {
            if (this.shouldShowOptionsSection_()) {
                console.error('[Brave Extensions Overrides] Could not find optionsSection (and it was meant to be shown). Has the Id changed?')
            }
            return
        }

        const incognitoSetting = optionsSection.querySelector('#allow-incognito')
        if (!incognitoSetting) {
            if (this.shouldShowIncognitoOption_()) {
                console.error('[Brave Extensions Overrides] Could not find incognitoSetting (and it was meant to be shown). Has the Id changed?')
            }
            return
        }

        const incognitoWarningDiv = incognitoSetting.querySelector('.section-content')
        if (!incognitoWarningDiv) {
            console.error('[Brave Extensions Overrides] Could not find incognitoWarningDiv with selector ".section-content"')
            return
        }

        incognitoWarningDiv.replace(html`<div class="section-content">
            ${loadTimeData.getString('privateInfoWarning')}
            <span ?hidden="${(this.data as any).isSplitMode}">
                ${loadTimeData.getString('spanningInfoWarning')}
            </span>
            <span>
                ${loadTimeData.getString('privateAndTorInfoWarning')}
            </span>
            <span>${this.data.incognitoAccess.isActive}</span>
        </div>`)
    })
}
