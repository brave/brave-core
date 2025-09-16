/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_radio_button/cr_radio_button.js'
import 'chrome://resources/cr_elements/cr_radio_group/cr_radio_group.js'
import 'chrome://resources/cr_elements/cr_shared_style.css.js'
import 'chrome://resources/cr_elements/cr_shared_vars.css.js'
import '../settings_shared.css.js'
import '../site_favicon.js'

import type {CrDialogElement} from 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {assert} from 'chrome://resources/js/assert.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {loadTimeData} from '../i18n_setup.js'

import type {SearchEngine, SearchEnginesBrowserProxy} from '../search_page/search_engines_browser_proxy.js'
import {ChoiceMadeLocation, SearchEnginesBrowserProxyImpl} from '../search_page/search_engines_browser_proxy.js'

import {getTemplate} from './normal_search_engine_list_dialog.html.js'

export interface SettingsNormalSearchEngineListDialogElement {
  $: {
    dialog: CrDialogElement,
  }
}

const SettingsNormalSearchEngineListDialogElementBase =
  WebUiListenerMixin(PolymerElement)

export class SettingsNormalSearchEngineListDialogElement extends
    SettingsNormalSearchEngineListDialogElementBase {
  static get is() {
    return 'settings-normal-search-engine-list-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /**
       * List of search engines available.
       */
      searchEngines: {
        type: Array,
        observer: 'searchEnginesChanged_',
      },

      /**
       * The id of the search engine that is selected by the user.
       */
      selectedEngineId_: {
        type: String,
        value: '',
      },

      /**
       * Whether the checkbox to save the search engine choice in guest mode
       * should be shown.
       */
      showSaveGuestChoice_: {
        type: Boolean,
        computed: 'computeShowSaveGuestChoice_(saveGuestChoice_)',
      },

      /**
       * State of the checkbox to save the search engine in guest mode. Null if
       * checkbox is not displayed.
       */
      saveGuestChoice_: {
        type: Boolean,
        value: null,
        notify: true,
      },

      /**
       * Promoted search engine
       */
       promotedEngine_: {
         type: Object,
         computed: 'computePromotedEngine_(searchEngines)',
       },
    }
  }

  declare searchEngines: SearchEngine[]

  private declare promotedEngine_: SearchEngine|undefined
  private declare selectedEngineId_: string
  private declare showSaveGuestChoice_: boolean
  private declare saveGuestChoice_: boolean|null
  private browserProxy_: SearchEnginesBrowserProxy =
      SearchEnginesBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()

    this.browserProxy_.getSaveGuestChoice().then(
      (saveGuestChoice: boolean|null) => {
        this.saveGuestChoice_ = saveGuestChoice
      })
  }

  private onSetAsDefaultClick_() {
    const searchEngine = this.searchEngines.find(
      engine => engine.id === parseInt(this.selectedEngineId_))
    assert(searchEngine)

    this.browserProxy_.setDefaultSearchEngine(
      searchEngine.modelIndex, ChoiceMadeLocation.SEARCH_SETTINGS,
      this.saveGuestChoice_)

    this.dispatchEvent(new CustomEvent('search-engine-changed', {
      bubbles: true,
      composed: true,
      detail: {
        searchEngine: searchEngine,
      },
    }))
    this.$.dialog.close()
  }

  private onCancelClick_() {
    this.$.dialog.close()
  }

  private searchEnginesChanged_() {
    if (!this.searchEngines.length) {
      return
    }

    const defaultSearchEngine =
        this.searchEngines.find(searchEngine => searchEngine.default)
    assert(defaultSearchEngine)
    this.selectedEngineId_ = defaultSearchEngine.id.toString()
  }

  private computeShowSaveGuestChoice_(saveGuestChoice: boolean|null): boolean {
    return saveGuestChoice !== null
  }

  private isPromotedEngine_(engine: SearchEngine): boolean {
    return this.promotedEngine_ === engine
  }

  private isBraveSearchEngine_(engine: SearchEngine): boolean {
    return engine.name === loadTimeData.getString('braveSearchEngineName')
        && engine.isPrepopulated === true
  }

  private computePromotedEngine_(
      searchEngines: Array<SearchEngine>): SearchEngine|undefined {
    if (loadTimeData.getBoolean('isLocaleJapan') || !searchEngines?.length) {
      return undefined
    }
    return searchEngines.find(this.isBraveSearchEngine_)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-normal-search-engine-list-dialog':
     SettingsNormalSearchEngineListDialogElement;
  }
}

customElements.define(
    SettingsNormalSearchEngineListDialogElement.is,
    SettingsNormalSearchEngineListDialogElement);
