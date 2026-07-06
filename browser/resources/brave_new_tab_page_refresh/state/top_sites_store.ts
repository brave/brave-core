/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateStore, StateStore } from '$web-common/state_store'
import {
  SponsoredSite,
  TopSite,
  TopSitesListKind,
} from 'gen/brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.m.js'

export { SponsoredSite, TopSite, TopSitesListKind }

// Shared by the "What is sponsored site?" context menu link and the
// sponsored site tooltip's "Learn more" link.
export const sponsoredSiteLearnMoreURL =
  'https://support.brave.app/hc/en-us/articles/47175807694989'

export interface TopSitesState {
  initialized: boolean

  maxCustomTopSites: number
  showTopSites: boolean
  showSponsoredSites: boolean
  topSitesListKind: TopSitesListKind
  topSites: TopSite[]
  sponsoredSites: SponsoredSite[]
  actions: TopSitesActions
}

export type TopSitesStore = StateStore<TopSitesState>

export function defaultTopSitesStore(): TopSitesStore {
  return createStateStore<TopSitesState>({
    initialized: false,
    maxCustomTopSites: 48,
    showTopSites: true,
    showSponsoredSites: true,
    topSitesListKind: TopSitesListKind.kMostVisited,
    topSites: [],
    sponsoredSites: [],
    actions: {
      setShowTopSites(showTopSites) {},
      setShowSponsoredSites(showSponsoredSites) {},
      setTopSitesListKind(listKind) {},
      addTopSite(url, title) {},
      updateTopSite(currentURL, newURL, title) {},
      removeTopSite(url) {},
      undoRemoveTopSite() {},
      setTopSitePosition(url, pos) {},
      recordTopSiteClick() {},
      disableSponsoredSites() {},
    },
  })
}

export interface TopSitesActions {
  setShowTopSites: (showTopSites: boolean) => void
  setShowSponsoredSites: (showSponsoredSites: boolean) => void
  setTopSitesListKind: (listKind: TopSitesListKind) => void
  addTopSite: (url: string, title: string) => void
  updateTopSite: (currentURL: string, newURL: string, title: string) => void
  removeTopSite: (url: string) => void
  undoRemoveTopSite: () => void
  setTopSitePosition: (url: string, pos: number) => void
  recordTopSiteClick: () => void
  disableSponsoredSites: () => void
}
