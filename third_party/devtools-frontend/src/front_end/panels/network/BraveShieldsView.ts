/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as LegacyWrapper from '../../ui/components/legacy_wrapper/legacy_wrapper.js'
import * as UI from '../../ui/legacy/legacy.js'
import { BraveShieldsCosmeticFiltersView } from './BraveShieldsCosmeticFiltersView.js'
import { BraveShieldsScriptletsView } from './BraveShieldsScriptletsView.js'

export class BraveShieldsView extends UI.Panel.Panel {
  #tabbedPane: UI.TabbedPane.TabbedPane
  #cosmeticFiltersView: BraveShieldsCosmeticFiltersView
  #scriptletsView: BraveShieldsScriptletsView

  constructor() {
    super('brave-shields')
    this.#tabbedPane = new UI.TabbedPane.TabbedPane()
    this.#tabbedPane.setCloseableTabs(false)
    this.#cosmeticFiltersView = new BraveShieldsCosmeticFiltersView()
    this.#scriptletsView = new BraveShieldsScriptletsView()

    this.#tabbedPane.appendTab(
      'cosmetic',
      'Cosmetic filters',
      LegacyWrapper.LegacyWrapper.legacyWrapper(
        UI.Widget.VBox,
        this.#cosmeticFiltersView
      )
    )

    this.#tabbedPane.appendTab(
      'scriptlets',
      'Scriptlets',
      LegacyWrapper.LegacyWrapper.legacyWrapper(
        UI.Widget.VBox,
        this.#scriptletsView
      )
    )

    this.#tabbedPane.show(this.element)
  }
}
