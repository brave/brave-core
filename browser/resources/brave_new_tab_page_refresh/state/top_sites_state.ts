/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  TopSite,
  TopSitesListKind,
} from 'gen/brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.m.js'

export { TopSite, TopSitesListKind }

export interface TopSitesState {
  initialized: boolean
  maxCustomTopSites: number
  showTopSites: boolean
  topSitesListKind: TopSitesListKind
  topSites: TopSite[]
}

export function defaultTopSitesState(): TopSitesState {
  return {
    initialized: false,
    maxCustomTopSites: 48,
    showTopSites: true,
    topSitesListKind: TopSitesListKind.kMostVisited,
    topSites: [],
  }
}

export interface TopSitesActions {
  setShowTopSites: (showTopSites: boolean) => void
  setTopSitesListKind: (listKind: TopSitesListKind) => void
  addTopSite: (url: string, title: string) => void
  updateTopSite: (currentURL: string, newURL: string, title: string) => void
  removeTopSite: (url: string) => void
  undoRemoveTopSite: () => void
  setTopSitePosition: (url: string, pos: number) => void
}

export function defaultTopSitesActions(): TopSitesActions {
  return {
    setShowTopSites(showTopSites) {},
    setTopSitesListKind(listKind) {},
    addTopSite(url, title) {},
    updateTopSite(currentURL, newURL, title) {},
    removeTopSite(url) {},
    undoRemoveTopSite() {},
    setTopSitePosition(url, pos) {},
  }
}
