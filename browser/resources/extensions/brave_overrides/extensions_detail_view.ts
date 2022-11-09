// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications, RegisterPolymerComponentBehaviors} from 'chrome://resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js'


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
    incognitoWarningDiv.innerText = I18nBehavior.i18n('privateInfoWarning')
    const spanningWarningSpan = document.createElement('span')
    spanningWarningSpan.setAttribute('class', 'section-content')
    spanningWarningSpan.setAttribute('hidden', '[[data.isSplitMode]]')
    spanningWarningSpan.innerText = ' ' +  I18nBehavior.i18n('spanningInfoWarning')
    const privateAndTorWarningSpan = document.createElement('span')
    privateAndTorWarningSpan.setAttribute('class', 'section-content')
    privateAndTorWarningSpan.innerText = ' ' + I18nBehavior.i18n('privateAndTorInfoWarning')
    incognitoWarningDiv.appendChild(spanningWarningSpan)
    incognitoWarningDiv.appendChild(privateAndTorWarningSpan)
  }
})
