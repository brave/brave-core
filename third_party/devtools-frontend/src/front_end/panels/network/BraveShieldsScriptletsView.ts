/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Common from '../../core/common/common.js'
import * as SDK from '../../core/sdk/sdk.js'
import * as ComponentHelpers from '../../ui/components/helpers/helpers.js'
import * as LegacyWrapper from '../../ui/components/legacy_wrapper/legacy_wrapper.js'
import * as LitHtml from '../../ui/lit-html/lit-html.js'
import * as BraveShieldsModel from './BraveShieldsModel.js'
import * as BraveModel from './BraveModel.js'

export class BraveShieldsScriptletsView
  extends LegacyWrapper.LegacyWrapper.WrappableComponent
  implements SDK.TargetManager.Observer
{
  static readonly litTagName = LitHtml.literal`devtools-brave-shields-scriptlets-view`
  readonly #shadow = this.attachShadow({ mode: 'open' })
  readonly #renderBound = this.#render.bind(this)
  #model?: BraveShieldsModel.BraveShieldsModel
  #scriptlets: string = ''

  constructor() {
    super()
    SDK.TargetManager.TargetManager.instance().observeTargets(this)

    SDK.TargetManager.TargetManager.instance().addModelListener(
      BraveShieldsModel.BraveShieldsModel,
      BraveModel.Events.BRAVE_EVENT_RECEIVED,
      this.#braveEventReceived,
      this,
      { scoped: true }
    )

    this.style.display = 'contents'
  }

  targetAdded(target: SDK.Target.Target): void {
    if (
      target === SDK.TargetManager.TargetManager.instance().primaryPageTarget()
    ) {
      this.#model = target.model(
        BraveShieldsModel.BraveShieldsModel
      ) as BraveShieldsModel.BraveShieldsModel
    }
  }

  targetRemoved(): void {}

  connectedCallback(): void {
    void ComponentHelpers.ScheduledRender.scheduleRender(
      this,
      this.#renderBound
    )
  }

  async #render(): Promise<void> {
    const style = LitHtml.Directives.styleMap({
      height: '100%',
      'white-space': 'pre',
      'padding-left': '10px',
      'user-select': 'text',
      cursor: 'auto'
    })

    LitHtml.render(
      LitHtml.html`
        <code style=${style}>
          ${this.#scriptlets}
        </code>
    `,
      this.#shadow,
      { host: this }
    )
  }

  #braveEventReceived({
    data
  }: Common.EventTarget.EventTargetEvent<BraveModel.BraveEvent>): void {
    if (this.#model !== data.braveModel) {
      return
    }
    this.#scriptlets = data.event.params.injected_script

    void ComponentHelpers.ScheduledRender.scheduleRender(
      this,
      this.#renderBound
    )
  }
}

customElements.define(
  'devtools-brave-shields-scriptlets-view',
  BraveShieldsScriptletsView
)

declare global {
  interface HTMLElementTagNameMap {
    'devtools-brave-shields-scriptlets-view': BraveShieldsScriptletsView
  }
}
