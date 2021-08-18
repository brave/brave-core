// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import 'chrome://resources/polymer/v3_0/iron-media-query/iron-media-query.js';
import 'chrome://brave-resources/br_elements/shared_vars_css.m.js';
import './br_toolbar_search_field.js';

const customCurrentWebUINameMap = {
  extensions: 'settings',
  sync: 'settings',
}

Polymer({
  is: 'cr-toolbar',

  _template: html`{__html_template__}`,

  properties: {
    // Name to display in the toolbar, in titlecase.
    pageName: String,

    // Prompt text to display in the search field.
    searchPrompt: String,

    // Tooltip to display on the clear search button.
    clearLabel: String,

    // Tooltip to display on the menu button.
    menuLabel: String,

    // Promotional toolstip string, shown in narrow mode if showMenuPromo is
    // true.
    menuPromo: String,

    // Value is proxied through to cr-toolbar-search-field. When true,
    // the search field will show a processing spinner.
    spinnerActive: Boolean,

    // Controls whether the menu button is shown at the start of the menu.
    showMenu: {type: Boolean, value: false},

    // Whether to show menu promo tooltip.
    showMenuPromo: {
      type: Boolean,
      value: false,
    },

    // Controls whether the search field is shown.
    showSearch: {type: Boolean, value: true},

    // True when the toolbar is displaying in narrow mode.
    narrow: {
      type: Boolean,
      reflectToAttribute: true,
      readonly: true,
      notify: true,
    },

    /**
     * The threshold at which the toolbar will change from normal to narrow
     * mode, in px.
     */
    narrowThreshold: {
      type: Number,
      value: 900,
    },

    closeMenuPromo: String,

    noSearch: {
      observer: 'noSearchChanged_',
      type: Boolean,
      // boolean props on html attributes must default to false
      value: false
    },

    /** @private */
    showingSearch_: {
      type: Boolean,
      reflectToAttribute: true,
    },
  },

  /** @return {!CrToolbarSearchFieldElement} */
  getSearchField: function() {
    return this.$.search;
  },

  /** @private */
  onMenuTap_: function() {
    console.debug('[br_toolbar] Not Implemented: onMenuTap_')
  },

  focusMenuButton() {
    console.debug('[br_toolbar] Not Implemented: focusMenuButton')
  },

  /** @return {boolean} */
  isMenuFocused() {
    console.debug('[br_toolbar] Not Implemented: isMenuFocused (no menuButton)')
    return false;
  },

  /** @private */
  noSearchChanged_: function () {
    this.updateSearchDisplayed_()
  },

  /** @private */
  updateSearchDisplayed_: function () {
    this.$.search.hidden = this.noSearch
  },

  /**
   * @param {string} title
   * @param {boolean} showMenuPromo
   * @return {string} The title if the menu promo isn't showing, else "".
   */
  titleIfNotShowMenuPromo_: function(title, showMenuPromo) {
    return showMenuPromo ? '' : title;
  },

  getNavItemSelectedClassName: function(itemName) {
    // which navigation item is the current page?
    let currentWebUIName = document.location.hostname
    // override name from hostname, if applicable
    if (customCurrentWebUINameMap[currentWebUIName])
      currentWebUIName = customCurrentWebUINameMap[currentWebUIName]
    // does it match the item calling this function?
    const itemWebUIName = itemName
    if (itemName === currentWebUIName)
      return '-selected'
    // not selected
    return ''
  },

  notifyIfExtraSlotFilled() {
    const slotIsFilled = this.toolbarExtraSlot.assignedNodes().length !== 0
    const classNameFilled = '-slot-filled'
    if (slotIsFilled) {
      this.toolbarExtraElement.classList.add(classNameFilled)
    } else {
      this.toolbarExtraElement.classList.remove(classNameFilled)
    }
  },

  initSlotFilledDetection: function() {
    // Style the 'extra items' slot only if it containts
    // content.
    const toolbarExtraElement = this.$$('.toolbar-extra')
    if (!toolbarExtraElement) {
      console.error('Could not find "toolbar-extra" element')
      return
    }
    const toolbarExtraSlot = toolbarExtraElement.querySelector('slot')
    if (!toolbarExtraSlot) {
      console.error('Could not find "toolbar-extra" slot')
      return
    }
    this.toolbarExtraElement = toolbarExtraElement
    this.toolbarExtraSlot = toolbarExtraSlot
    this.notifyIfExtraSlotFilled()
    toolbarExtraSlot.addEventListener('slotchange',  (e) => {
      this.notifyIfExtraSlotFilled()
    })
  },

  initStrings: function() {
    this.historyTitle = loadTimeData.getString('brToolbarHistoryTitle')
    this.settingsTitle = loadTimeData.getString('brToolbarSettingsTitle')
    this.bookmarksTitle = loadTimeData.getString('brToolbarBookmarksTitle')
    this.downloadsTitle = loadTimeData.getString('brToolbarDownloadsTitle')
    this.braveRewardsTitle = loadTimeData.getString('brToolbarRewardsTitle')
    this.walletsTitle = loadTimeData.getString('brToolbarWalletsTitle')
  },

  /** @override */
  attached: function() {
    this.initSlotFilledDetection()
    this.initStrings()
    this.initFontLoadDetection()
  },

  initFontLoadDetection: async function() {
    // Font detection.
    // Wait for component to render with font style.
    await new Promise(resolve => window.requestAnimationFrame(resolve))
    const fontFaceString = '300 16px Poppins'
    if (document.fonts.check(fontFaceString)) {
      console.debug('fonts were loaded on init')
      this.setFontsAreLoaded()
      return
    }
    await document.fonts.ready
    if (!document.fonts.check(fontFaceString)) {
      console.debug('warning: fonts ready but string not set!')
    } else {
      console.debug('font was loaded after a wait')
    }
    this.setFontsAreLoaded()
  },

  setFontsAreLoaded: function() {
    this.fontsLoadedClassName = 'fonts-loaded'
  }
});
