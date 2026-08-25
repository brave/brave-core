/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { TopSitesContext } from '../../context/top_sites_context'
import {
  sponsoredSiteLearnMoreURL,
  TopSitesListKind,
  TopSitesState,
} from '../../state/top_sites_store'
import { TopSitesPanel } from './top_sites_panel'

// The default $web-common/locale mock (see components/test/testSetup.ts)
// echoes the string key back verbatim, which has no $1 placeholder for
// formatString to replace. Override just the sponsored sites description so
// it renders with its "Learn more" link as it would in production, while
// every other string key keeps echoing back for the other assertions below.
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    if (key === 'NEW_TAB_SPONSORED_SITES_DESCRIPTION') {
      return 'New tab page ads help keep Brave free. $1Learn more/$1.'
    }
    return key
  },
}))

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

  it('should render the sponsored sites description with a learn more link', () => {
    renderPanel({ showTopSites: true })
    expect(
      screen.getByText('New tab page ads help keep Brave free.', {
        exact: false,
      }),
    ).toBeInTheDocument()
    expect(screen.getByRole('link', { name: 'Learn more' })).toHaveAttribute(
      'href',
      sponsoredSiteLearnMoreURL,
    )
  })

  it('should stop the learn more link click from reaching the toggle', () => {
    // The Toggle wraps its label content in a native <label>, which
    // forwards an unstopped click to its associated control. Rather than
    // relying on jsdom to replicate that native label activation
    // behavior (uncertain in this test environment), this verifies the
    // fix's actual mechanism directly: the click must not bubble past the
    // description's wrapping element.
    renderPanel({ showTopSites: true })
    const onDocumentClick = jest.fn()
    document.addEventListener('click', onDocumentClick)
    try {
      screen.getByRole('link', { name: 'Learn more' }).click()
    } finally {
      document.removeEventListener('click', onDocumentClick)
    }
    expect(onDocumentClick).not.toHaveBeenCalled()
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
