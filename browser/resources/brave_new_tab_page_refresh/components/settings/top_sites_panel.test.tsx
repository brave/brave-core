/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { TopSitesContext } from '../../context/top_sites_context'
import { TopSitesListKind, TopSitesState } from '../../state/top_sites_store'
import { TopSitesPanel } from './top_sites_panel'

function createStore(
  state: Partial<TopSitesState>,
  actions: Partial<TopSitesState['actions']> = {},
) {
  return createStateStore<TopSitesState>({
    initialized: true,
    maxCustomTopSites: 48,
    showSponsoredSites: true,
    showTopSites: true,
    topSitesListKind: TopSitesListKind.kMostVisited,
    sponsoredSites: [],
    topSites: [],
    actions: {
      setShowSponsoredSites() {},
      setShowTopSites() {},
      setTopSitesListKind() {},
      addTopSite() {},
      updateTopSite() {},
      removeTopSite() {},
      undoRemoveTopSite() {},
      setTopSitePosition() {},
      recordTopSiteClick() {},
      ...actions,
    },
    ...state,
  })
}

function renderPanel(
  state: Partial<TopSitesState>,
  actions: Partial<TopSitesState['actions']> = {},
) {
  const store = createStore(state, actions)
  render(
    <TopSitesContext.Provider value={store}>
      <TopSitesPanel />
    </TopSitesContext.Provider>,
  )
}

describe('TopSitesPanel', () => {
  it('should not render the sponsored sites toggle when top sites are hidden', () => {
    renderPanel({ showTopSites: false })
    expect(
      screen.queryByText('NEW_TAB_SHOW_SPONSORED_SITES_LABEL'),
    ).not.toBeInTheDocument()
  })

  it('should render the sponsored sites toggle when top sites are shown', () => {
    renderPanel({ showTopSites: true })
    expect(
      screen.getByText('NEW_TAB_SHOW_SPONSORED_SITES_LABEL'),
    ).toBeInTheDocument()
  })

  it('should update the sponsored sites setting when its toggle changes', () => {
    const setShowSponsoredSites = jest.fn()
    renderPanel(
      { showTopSites: true, showSponsoredSites: false },
      { setShowSponsoredSites },
    )
    screen.getByText('NEW_TAB_SHOW_SPONSORED_SITES_LABEL').click()
    expect(setShowSponsoredSites).toHaveBeenCalledWith(true)
  })

  it('should update the top sites setting when its toggle changes', () => {
    const setShowTopSites = jest.fn()
    renderPanel({ showTopSites: false }, { setShowTopSites })
    screen.getByText('NEW_TAB_SHOW_TOP_SITES_LABEL').click()
    expect(setShowTopSites).toHaveBeenCalledWith(true)
  })

  it('should switch to the custom list view when that option is clicked', () => {
    const setTopSitesListKind = jest.fn()
    renderPanel(
      { showTopSites: true, topSitesListKind: TopSitesListKind.kMostVisited },
      { setTopSitesListKind },
    )
    screen.getByText('NEW_TAB_TOP_SITES_CUSTOM_OPTION_TITLE').click()
    expect(setTopSitesListKind).toHaveBeenCalledWith(TopSitesListKind.kCustom)
  })

  it('should switch to the most visited list view when that option is clicked', () => {
    const setTopSitesListKind = jest.fn()
    renderPanel(
      { showTopSites: true, topSitesListKind: TopSitesListKind.kCustom },
      { setTopSitesListKind },
    )
    screen.getByText('NEW_TAB_TOP_SITES_MOST_VISITED_OPTION_TITLE').click()
    expect(setTopSitesListKind).toHaveBeenCalledWith(
      TopSitesListKind.kMostVisited,
    )
  })
})
