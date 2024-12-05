// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';

RegisterPolymerTemplateModifications({
  'extensions-detail-view': (templateContent: DocumentFragment) => {
    let optionsTemplate: HTMLTemplateElement | null =
      templateContent.querySelector<HTMLTemplateElement>('template[is="dom-if"][if*="shouldShowOptionsSection_"]')
    if (!optionsTemplate) {
      console.error('[Brave Extensions Overrides] Could not find optionsTemplate')
      return
    }
    let incognitoTemplate =
      optionsTemplate.content.querySelector<HTMLTemplateElement>('template[is="dom-if"][if*="shouldShowIncognitoOption_"]')
    if (!incognitoTemplate) {
      console.error('[Brave Extensions Overrides] Could not find incognitoTemplate')
      return
    }
    let incognitoWarningDiv = incognitoTemplate.content.querySelector<HTMLDivElement>('.section-content')
    if (!incognitoWarningDiv) {
      console.error('[Brave Extensions Overrides] Could not find incognitoWarningDiv')
      return
    }
    incognitoWarningDiv.innerText = loadTimeData.getString('privateInfoWarning')
    const spanningWarningSpan = document.createElement('span')
    spanningWarningSpan.setAttribute('class', 'section-content')
    spanningWarningSpan.setAttribute('hidden', '[[data.isSplitMode]]')
    spanningWarningSpan.innerText =
        ' ' + loadTimeData.getString('spanningInfoWarning')
    const privateAndTorWarningSpan = document.createElement('span')
    privateAndTorWarningSpan.setAttribute('class', 'section-content')
    privateAndTorWarningSpan.innerText =
        ' ' + loadTimeData.getString('privateAndTorInfoWarning')
    incognitoWarningDiv.appendChild(spanningWarningSpan)
    incognitoWarningDiv.appendChild(privateAndTorWarningSpan)
  }
})

export * from './detail_view-chromium.js'
