/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as UI from '../../ui/legacy/legacy.js'
import * as Lit from '../../ui/lit/lit.js'

import type * as NetworkForward from './forward/forward.js'
import type * as NetworkItemViewModule from './NetworkItemView.js'
import { RequestAdblockView } from './RequestAdblockView.js'

const { html, nothing, render } = Lit
const { widget } = UI.Widget

export const ADBLOCK_TAB_ID =
  'brave-adblock' as NetworkForward.UIRequestLocation.UIRequestTabs

export function PatchNetworkItemView(
  NetworkItemView: typeof NetworkItemViewModule.NetworkItemView,
  defaultView: NetworkItemViewModule.View
): typeof NetworkItemViewModule.NetworkItemView {
  return class extends NetworkItemView {
    constructor(
      request: NetworkItemViewModule.ViewInput['request'],
      calculator: NetworkItemViewModule.ViewInput['calculator'],
      initialTab?: NetworkForward.UIRequestLocation.UIRequestTabs,
      view: NetworkItemViewModule.View = defaultView
    ) {
      super(request, calculator, initialTab, (input, output, target) => {
        view(input, output, target)
        const tabbedPane = target.querySelector('devtools-tabbed-pane')
        if (!tabbedPane) {
          return
        }
        const selected = input.selectedTab === ADBLOCK_TAB_ID
        render(
          html`
            <div
              id=${ADBLOCK_TAB_ID}
              class="vbox flex-auto"
              label="Adblock"
              tab-tooltip="Adblock"
              ?selected=${Lit.Directives.live(selected)}>
              ${selected
                ? widget(RequestAdblockView, { request: input.request })
                : nothing}
            </div>
          `,
          tabbedPane
        )
      })
    }
  }
}
