/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from 'chai'
import { beforeEach, it } from 'mocha'

import * as NetworkTimeCalculator from '../../models/network_time_calculator/network_time_calculator.js'
import { renderElementIntoDOM } from '../../testing/DOMHelpers.js'
import { describeWithEnvironment } from '../../testing/EnvironmentHelpers.js'
import { createNetworkRequest } from '../../testing/NetworkRequestHelpers.js'
import { setUpEnvironment } from '../../testing/OverridesHelpers.js'
import * as UI from '../../ui/legacy/legacy.js'

import * as NetworkForward from './forward/forward.js'
import * as Network from './network.js'

const ADBLOCK_TAB_ID = 'brave-adblock'
const HEADERS_COMPONENT =
  NetworkForward.UIRequestLocation.UIRequestTabs.HEADERS_COMPONENT
const TIMING = NetworkForward.UIRequestLocation.UIRequestTabs.TIMING

async function renderNetworkItemView(url: string): Promise<{
  networkItemView: Network.NetworkItemView.NetworkItemView
  tabbedPane: UI.TabbedPane.TabbedPane
  tabContent: (id: string) => Element
}> {
  const request = createNetworkRequest({ url, finished: true })
  const networkItemView = new Network.NetworkItemView.NetworkItemView(
    request,
    new NetworkTimeCalculator.NetworkTimeCalculator(true)
  )
  renderElementIntoDOM(networkItemView)
  await UI.Widget.Widget.allUpdatesComplete
  const tabbedPaneElement =
    networkItemView.contentElement.querySelector<UI.TabbedPane.TabbedPaneElement>(
      'devtools-tabbed-pane'
    )
  assert.exists(tabbedPaneElement)
  const tabbedPane = tabbedPaneElement.getWidget()
  assert.exists(tabbedPane)
  return {
    networkItemView,
    tabbedPane,
    tabContent: (id) => {
      const content = tabbedPaneElement.querySelector(`:scope > [id="${id}"]`)
      assert.exists(content)
      return content
    }
  }
}

async function selectTab(tabbedPane: UI.TabbedPane.TabbedPane, id: string) {
  assert.isTrue(tabbedPane.selectTab(id, /* userGesture=*/ true))
  await UI.Widget.Widget.allUpdatesComplete
}

describeWithEnvironment('NetworkItemView (Brave)', () => {
  beforeEach(() => {
    setUpEnvironment()
  })

  it("adds the Adblock tab after upstream's tabs", async () => {
    const { tabbedPane } = await renderNetworkItemView(
      'https://www.example.com/a.js'
    )

    const tabIds = tabbedPane.tabIds()
    assert.include(tabIds, HEADERS_COMPONENT)
    assert.strictEqual(tabIds.at(-1), ADBLOCK_TAB_ID)
  })

  it('shows the Adblock view only while its tab is selected', async () => {
    const { tabbedPane, tabContent } = await renderNetworkItemView(
      'https://www.example.com/a.js'
    )

    await selectTab(tabbedPane, ADBLOCK_TAB_ID)
    assert.strictEqual(tabbedPane.selectedTabId, ADBLOCK_TAB_ID)
    assert.isNotNull(tabContent(ADBLOCK_TAB_ID).firstElementChild)
    assert.isNull(tabContent(HEADERS_COMPONENT).firstElementChild)

    await selectTab(tabbedPane, HEADERS_COMPONENT)
    assert.strictEqual(tabbedPane.selectedTabId, HEADERS_COMPONENT)
    assert.isNull(tabContent(ADBLOCK_TAB_ID).firstElementChild)
    assert.isNotNull(tabContent(HEADERS_COMPONENT).firstElementChild)
  })

  it('returns to the Adblock tab from another tab', async () => {
    const { tabbedPane, tabContent } = await renderNetworkItemView(
      'https://www.example.com/a.js'
    )

    await selectTab(tabbedPane, ADBLOCK_TAB_ID)
    await selectTab(tabbedPane, TIMING)
    await selectTab(tabbedPane, ADBLOCK_TAB_ID)

    assert.strictEqual(tabbedPane.selectedTabId, ADBLOCK_TAB_ID)
    assert.isNotNull(tabContent(ADBLOCK_TAB_ID).firstElementChild)
    assert.isNull(tabContent(TIMING).firstElementChild)
  })

  it("opens the next request's details on the Adblock tab when it was selected last", async () => {
    const first = await renderNetworkItemView('https://www.example.com/a.js')
    await selectTab(first.tabbedPane, ADBLOCK_TAB_ID)
    first.networkItemView.detach()

    const next = await renderNetworkItemView('https://www.example.com/b.js')

    assert.strictEqual(next.tabbedPane.selectedTabId, ADBLOCK_TAB_ID)
    assert.isNotNull(next.tabContent(ADBLOCK_TAB_ID).firstElementChild)
  })
})
