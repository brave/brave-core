// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications, RegisterPolymerComponentBehaviors} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'

RegisterPolymerTemplateModifications({
  'extensions-detail-view': (templateContent) => {
    let optionsTemplate =
      templateContent.querySelector('template[is="dom-if"][if*="shouldShowOptionsSection_"]')
    if (!optionsTemplate) {
      console.error('[Brave Extensions Overrides] Could not find optionsTemplate')
      return
    }
    let incognitoTemplate =
      optionsTemplate.content.querySelector('template[is="dom-if"][if*="shouldShowIncognitoOption_"]')
    if (!incognitoTemplate) {
      console.error('[Brave Extensions Overrides] Could not find incognitoTemplate')
      return
    }

    incognitoTemplate.insertAdjacentHTML('afterend', `
      <template is="dom-if"
        if="[[data.torAccess.isEnabled]]">
        <extensions-toggle-row id="allow-tor"
            checked="[[data.torAccess.isActive]]"
            class="hr"
            on-change="onAllowTorChange_">
          <div>
            <div>${I18nBehavior.i18n('itemAllowInTor')}</div>
            <div class="section-content">${I18nBehavior.i18n('torInfoWarning')}</div>
          </div>
        </extensions-toggle-row>
        </template>
     `)
  }
})

RegisterPolymerComponentBehaviors({
  'extensions-detail-view': [{
    /** @private */
    onAllowTorChange_: function() {
      this.delegate.setItemAllowedTor(
        this.data.id, this.$$('#allow-tor').checked);
    },
  }]
})
