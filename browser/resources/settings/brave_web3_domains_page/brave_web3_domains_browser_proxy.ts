// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {DropdownMenuOptionList} from '/shared/settings/controls/settings_dropdown_menu.js';
import {sendWithPromise} from 'chrome://resources/js/cr.js'

import {loadTimeData} from '../i18n_setup.js';

export interface BraveWeb3DomainsBrowserProxy {
  getDecentralizedDnsResolveMethodList(): Promise<DropdownMenuOptionList>
  getEnsOffchainResolveMethodList(): Promise<DropdownMenuOptionList>
  isENSL2Enabled(): boolean
  isSnsEnabled(): boolean
}

export class BraveWeb3DomainsBrowserProxyImpl
  implements BraveWeb3DomainsBrowserProxy {
  getDecentralizedDnsResolveMethodList() {
    return sendWithPromise('getDecentralizedDnsResolveMethodList')
  }

  getEnsOffchainResolveMethodList() {
    return sendWithPromise('getEnsOffchainResolveMethodList')
  }

  isENSL2Enabled() {
    return loadTimeData.getBoolean('isENSL2Enabled')
  }

  isSnsEnabled() {
    return loadTimeData.getBoolean('isSnsEnabled')
  }

  static getInstance(): BraveWeb3DomainsBrowserProxy {
    return instance || (instance = new BraveWeb3DomainsBrowserProxyImpl())
  }
}

let instance: BraveWeb3DomainsBrowserProxy|null = null
