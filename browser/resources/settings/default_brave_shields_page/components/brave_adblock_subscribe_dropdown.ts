/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {BaseMixin} from '../../base_mixin.js';
import {PrefsMixin} from '../../prefs/prefs_mixin.js';
import {getTemplate} from './brave_adblock_subscribe_dropdown.html.js'

import 'chrome://resources/cr_elements/cr_action_menu/cr_action_menu.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/cr_elements/icons.html.js';

const AdblockSubscribeDropDownBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class AdBlockSubscribeDropDown extends AdblockSubscribeDropDownBase {
  static get is() {
    return 'adblock-subscribe-dropdown'
  }

  static get template() {
    return getTemplate()
  }

  ready() {
    super.ready()
  }

  handleDropdownClick_(e) {
    this.$.menu.showAt(e.target)
  }

  updateSubscription_() {
    this.fire('update-subscription')
    this.$.menu.close()
  }

  viewSubscription_() {
    this.fire('view-subscription')
    this.$.menu.close()
  }

  deleteSubscription_() {
    this.fire('delete-subscription')
    this.$.menu.close()
  }
}

customElements.define(AdBlockSubscribeDropDown.is, AdBlockSubscribeDropDown)
