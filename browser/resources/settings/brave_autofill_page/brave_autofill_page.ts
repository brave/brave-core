// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {SettingsAutofillPageElement} from '../autofill_page/autofill_page.js'
import {Router, RouteObserverMixin, Route} from '../router.js'

const BaseElement = RouteObserverMixin(SettingsAutofillPageElement)

export class BraveSettingsAutofillPageElement extends BaseElement {
  static override get properties() {
    const baseProperties = super.properties
    const extended = Object.assign({}, baseProperties, {
      isAutofillPage_: Boolean
    })
    return extended
  }

  private isAutofillPage_: boolean

  override currentRouteChanged(newRoute: Route) {
    this.isAutofillPage_ = newRoute == Router.getInstance().getRoutes().AUTOFILL
  }
}
