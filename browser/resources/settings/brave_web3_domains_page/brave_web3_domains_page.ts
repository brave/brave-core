// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {DropdownMenuOptionList} from '/shared/settings/controls/settings_dropdown_menu.js';
import {PrefsMixin, PrefsMixinInterface} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BraveWeb3DomainsBrowserProxyImpl} from './brave_web3_domains_browser_proxy.js'
import {getTemplate} from './brave_web3_domains_page.html.js'

const SettingBraveWeb3DomainsPageElementBase = PrefsMixin(PolymerElement) as {
  new (): PolymerElement & PrefsMixinInterface
}

export class SettingBraveWeb3DomainsPageElement
  extends SettingBraveWeb3DomainsPageElementBase {
  static get is() {
    return 'settings-brave-web3-domains-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      showSnsRow_: Boolean,
      resolveMethod_: Array,
      ensOffchainResolveMethod_: Array,
      showEnsOffchainLookupRow_: {
        type: Boolean,
        computed: 'computeShowEnsOffchainLookupRow_(prefs.*)',
      },
    }
  }

  private browserProxy_ = BraveWeb3DomainsBrowserProxyImpl.getInstance()
  showSnsRow_: boolean
  resolveMethod_: DropdownMenuOptionList
  ensOffchainResolveMethod_: DropdownMenuOptionList

  override ready() {
    super.ready()

    this.showSnsRow_ = this.browserProxy_.isSnsEnabled()
    this.browserProxy_.getDecentralizedDnsResolveMethodList().then(list => {
      this.resolveMethod_ = list
    })
    this.browserProxy_.getEnsOffchainResolveMethodList().then(list => {
      this.ensOffchainResolveMethod_ = list
    })
  }

  computeShowEnsOffchainLookupRow_() {
    if (!this.browserProxy_.isENSL2Enabled())
      return false
    return !!this.prefs && this.getPref('brave.ens.resolve_method').value === 3
  }
}

customElements.define(
  SettingBraveWeb3DomainsPageElement.is, SettingBraveWeb3DomainsPageElement)
