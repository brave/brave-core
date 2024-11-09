// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {SettingsAutofillPageElement} from '../autofill_page/autofill_page.js'
import {Router, RouteObserverMixin, Route} from '../router.js'

type PolymerConstructor = new (...args: any[]) => PolymerElement;
const BaseElement: PolymerConstructor = SettingsAutofillPageElement

export class BraveSettingsAutofillPageElement extends RouteObserverMixin(BaseElement) {
  static get properties() {
    return {
      isAutofillPage_: Boolean
    }
  }

  isAutofillPage_: boolean

  override currentRouteChanged(newRoute: Route) {
    this.isAutofillPage_ = newRoute == Router.getInstance().getRoutes().AUTOFILL
  }

  private onEmailAliasesClicked_() {
    const router = Router.getInstance()
    router.navigateTo((router.getRoutes() as any).EMAIL_ALIASES)
  }
}
