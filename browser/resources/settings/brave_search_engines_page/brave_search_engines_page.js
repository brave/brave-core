/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import '../settings_shared_css.js';
import '../settings_vars_css.js';

import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUIListenerMixin} from 'chrome://resources/js/web_ui_listener_mixin.js';
import {loadTimeData} from "../i18n_setup.js"
import {BraveSearchEnginesPageBrowserProxyImpl} from './brave_search_engines_page_browser_proxy.m.js';

const BraveSearchEnginesPageBase = WebUIListenerMixin(PolymerElement)

class BraveSearchEnginesPage extends BraveSearchEnginesPageBase {
  static get is() {
    return 'settings-brave-search-page'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      shouldShowPrivateSearchProvider_: {
        readOnly: true,
        type: Boolean,
        value: !loadTimeData.getBoolean('isGuest')
      },

      privateSearchEngines_: {
        readOnly: false,
        type: Array
      }
    }
  }

  browserProxy_ = BraveSearchEnginesPageBrowserProxyImpl.getInstance()

  ready() {
    super.ready()

    const updatePrivateSearchEngines = list => {
      this.set('privateSearchEngines_', list)
    }

    this.browserProxy_.getPrivateSearchEnginesList().then(updatePrivateSearchEngines)
    this.addWebUIListener('private-search-engines-changed', updatePrivateSearchEngines)
  }
}

customElements.define(BraveSearchEnginesPage.is, BraveSearchEnginesPage);
