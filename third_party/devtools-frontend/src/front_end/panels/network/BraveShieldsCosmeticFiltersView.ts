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
import * as SplitView from '../../ui/components/split_view/split_view.js'
import * as VisualLogging from '../../ui/visual_logging/visual_logging.js'
import * as DataGrid from '../../ui/components/data_grid/data_grid.js'
import * as Input from '../../ui/components/input/input.js'

export class BraveShieldsCosmeticFiltersView
  extends LegacyWrapper.LegacyWrapper.WrappableComponent
  implements SDK.TargetManager.Observer
{
  readonly #shadow = this.attachShadow({ mode: 'open' })
  readonly #renderBound = this.#render.bind(this)
  private shieldsModel?: BraveShieldsModel.BraveShieldsModel | null
  #activeFilters: BraveShieldsModel.Filter[] | null = null
  #focusedFilter: BraveShieldsModel.Filter | null = null

  private target?: SDK.Target.Target
  private resourceTreeModel?: SDK.ResourceTreeModel.ResourceTreeModel | null
  private registeredListeners: Common.EventTarget.EventDescriptor[]
  private shieldsData: any

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

    this.registeredListeners = []
    this.style.display = 'contents'
  }

  targetAdded(target: SDK.Target.Target): void {
    if (
      target === SDK.TargetManager.TargetManager.instance().primaryPageTarget()
    ) {
      this.resourceTreeModel = target.model(
        SDK.ResourceTreeModel.ResourceTreeModel
      )
      this.shieldsModel = target.model(
        BraveShieldsModel.BraveShieldsModel
      ) as BraveShieldsModel.BraveShieldsModel

      if (!this.resourceTreeModel || !this.shieldsModel) {
        return
      }

      this.registeredListeners = [
        this.resourceTreeModel.addEventListener(
          SDK.ResourceTreeModel.Events.DOMContentLoaded,
          () => {
            this.updateFilters()
          }
        )
      ]
    }
  }

  targetRemoved(target: SDK.Target.Target): void {
    if (this.target !== target) {
      return
    }
    if (!this.resourceTreeModel || !this.shieldsModel) {
      return
    }
    delete this.resourceTreeModel
    delete this.shieldsModel
    Common.EventTarget.removeEventListeners(this.registeredListeners)
  }

  connectedCallback(): void {
    this.#shadow.adoptedStyleSheets = [Input.checkboxStyles]
    void ComponentHelpers.ScheduledRender.scheduleRender(
      this,
      this.#renderBound
    )
  }

  #onFilterFocused(event: Event): void {
    const focusedEvent = event as DataGrid.DataGridEvents.BodyCellFocusedEvent
    const cell = focusedEvent.data.cell
    if (!cell || !this.#activeFilters) {
      this.#focusedFilter = null
    } else {
      this.#focusedFilter = this.#activeFilters[cell.value as number]
    }
    void ComponentHelpers.ScheduledRender.scheduleRender(
      this,
      this.#renderBound
    )
  }

  #onNodeFocused(event: Event): void {}

  #buildFilters(): DataGrid.DataGridUtils.Row[] {
    if (!this.#activeFilters) {
      return []
    }
    return this.#activeFilters.map((filter, index) => {
      return {
        cells: [
          {
            columnId: 'filter',
            value: `${index}`,
            renderer: () => {
              const style = {
                ...(filter.nodes && { color: 'green' })
              }
              return LitHtml.html`
                <label class="checkbox-label" title=${filter.selector}>
                  <input type="checkbox" ?checked=${!!filter.nodes}>
                </label>
                <span style=${LitHtml.Directives.styleMap(style)}>
                  ${filter.selector}
                </span>                
              `
            }
          }
        ],
        styles: {
          'font-family': 'var(--monospace-font-family)',
          'font-size': 'var(--monospace-font-size)'
        }
      }
    })
  }

  #renderFilters(): LitHtml.LitTemplate {
    const gridData: DataGrid.DataGridController.DataGridControllerData = {
      columns: [
        {
          id: 'filter',
          title: 'Filter',
          widthWeighting: 100,
          hideable: false,
          visible: true,
          sortable: false
        }
      ],
      striped: true,
      rows: this.#buildFilters()
    }

    const style = LitHtml.Directives.styleMap({
      'border-top': '1px solid var(--sys-color-divider)',
      'box-sizing': 'border-box'
    })

    return LitHtml.html`
        <devtools-data-grid-controller
          style=${style}
          .data=${
            gridData as DataGrid.DataGridController.DataGridControllerData
          }
          @cellfocused=${this.#onFilterFocused}
        >
        </devtools-data-grid-controller>
    `
  }

  #buildNodes(): DataGrid.DataGridUtils.Row[] {
    if (!this.#focusedFilter || !this.#focusedFilter.nodes) {
      return []
    }

    const rows: DataGrid.DataGridUtils.Row[] = []
    for (const [index, node] of this.#focusedFilter.nodes.entries()) {
      rows.push({
        cells: [
          {
            columnId: 'node',
            value: `${index}`,
            renderer: () => {
              return LitHtml.html`
                <div>
                  ${LitHtml.Directives.until(
                    Common.Linkifier.Linkifier.linkify(node)
                  )}
                </div>
              `
            }
          }
        ],
        styles: {
          'font-family': 'var(--monospace-font-family)',
          'font-size': 'var(--monospace-font-size)'
        }
      })
    }
    return rows
  }

  #renderNodes(): LitHtml.LitTemplate {
    const gridData: DataGrid.DataGridController.DataGridControllerData = {
      columns: [
        {
          id: 'node',
          title: 'Node',
          widthWeighting: 100,
          hideable: false,
          visible: true,
          sortable: false
        }
      ],
      striped: true,
      rows: this.#buildNodes()
    }

    const style = LitHtml.Directives.styleMap({
      'border-top': '1px solid var(--sys-color-divider)',
      'box-sizing': 'border-box'
    })

    return LitHtml.html`
        <devtools-data-grid-controller
          style=${style}
          .data=${
            gridData as DataGrid.DataGridController.DataGridControllerData
          }
          @cellfocused=${this.#onNodeFocused}
        >
        <devtools-data-grid-controller>
    `
  }

  updateFilters(): void {
    if (!this.shieldsData || !this.shieldsModel) {
      return
    }
    const hideSelectors = this.shieldsData.hide_selectors
    const forceHideSelectors = this.shieldsData.force_hide_selectors
    this.shieldsModel
      .getActiveFilters(hideSelectors.concat(forceHideSelectors))
      .then((filters) => {
        this.#activeFilters = filters
      })
      .catch((e) => {
        this.#activeFilters = null
      })
      .finally(() => {
        void ComponentHelpers.ScheduledRender.scheduleRender(
          this,
          this.#renderBound
        )
      })
  }

  async #render(): Promise<void> {
    LitHtml.render(
      LitHtml.html`
      <devtools-split-view>
        <div slot="main">
          ${this.#renderFilters()}
        </div>
        <div slot="sidebar">
          ${this.#renderNodes()}
        </div>
      </devtools-split-view>
    `,
      this.#shadow,
      { host: this }
    )
  }

  #braveEventReceived({
    data
  }: Common.EventTarget.EventTargetEvent<BraveModel.BraveEvent>): void {
    if (this.shieldsModel !== data.braveModel) {
      return
    }
    this.shieldsData = data.event.params
    this.updateFilters()
  }
}

customElements.define(
  'devtools-brave-shields-cosmetic-filters-view',
  BraveShieldsCosmeticFiltersView
)

declare global {
  interface HTMLElementTagNameMap {
    'devtools-brave-shields-cosmetic-filters-view': BraveShieldsCosmeticFiltersView
  }
}
