// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html, RegisterPolymerTemplateModifications, RegisterPolymerComponentBehaviors} from 'chrome://resources/brave/polymer_overriding.js'
import {loadTimeData} from '../i18n_setup.js'
import '../brave_system_page/brave_performance_page.js'
import '../shortcuts_page/shortcuts_page.js'
import {Router} from '../router.js';

RegisterPolymerComponentBehaviors({
  'settings-system-page': [
    {
      onShortcutsClicked_: () => {
        const router = Router.getInstance();
        router.navigateTo((router.getRoutes() as any).SHORTCUTS)
      }
    }
  ]
})

RegisterPolymerTemplateModifications({
  'settings-system-page': (templateContent) => {
    if (loadTimeData.getBoolean('areShortcutsSupported')) {
      // Get all of the non-style children - we want to move these into the default
      // route, rather than always showing, otherwise, when we navigate, we'll get
      // all the toggles showing up.
      const nonStyleChildren = (Array.from(templateContent.children) as HTMLElement[])
        .filter(t => t.tagName !== 'STYLE');

      templateContent.appendChild(html`
        <settings-animated-pages id="pages" section="system">
          <div route-path="default">
            <cr-link-row on-click="onShortcutsClicked_" id="shortcutsButton" label=${loadTimeData.getString('braveShortcutsPage')} role-description="Subpage button">
              <span id="shortcutsButtonSubLabel" slot="sub-label">
            </cr-link-row>
            <div class="hr"></div>
          </div>
          <template is="dom-if" route-path="/system/shortcuts">
            <settings-subpage associated-control="[[$$('#shortcutsButton')]]" page-title=${loadTimeData.getString('braveShortcutsPage')}>
              <settings-shortcuts-page></settings-shortcuts-page>
            </settings-subpage>
          </template>
        </settings-animated-pages>`)
      const defaultRoute = templateContent.querySelector('#pages div[route-path=default]')

      for (const child of nonStyleChildren) {
        defaultRoute.appendChild(child);
      }

      // changes should happen inside the default route.
      templateContent = defaultRoute;
    }

    templateContent.appendChild(
      html`
        <settings-toggle-button
          class="cr-row"
          pref="{{prefs.brave.enable_closing_last_tab}}"
          label="${loadTimeData.getString("braveHelpTipsClosingLastTab")}">
        </settings-toggle-button>

        <settings-brave-performance-page prefs="{{prefs}}">
        </settings-brave-performance-page>
      `)
  }
})
