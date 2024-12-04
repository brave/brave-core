/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export interface TopSite {
  title: string
  url: string
  favicon: string
}

export type TopSitesListKind = 'custom' | 'most-visited'

export interface TopSitesState {
  showTopSites: boolean
  listKind: TopSitesListKind
  topSites: TopSite[]
}

export function defaultState(): TopSitesState {
  return {
    showTopSites: true,
    listKind: 'most-visited',
    topSites: []
  }
}

export interface TopSitesModel {
  getState: () => TopSitesState
  addListener: (listener: (state: TopSitesState) => void) => () => void
  setShowTopSites: (showTopSites: boolean) => void
  setListKind: (listKind: TopSitesListKind) => void
  addTopSite: (url: string, title: string) => void
  updateTopSite: (currentURL: string, newURL: string, title: string) => void
  removeTopSite: (url: string) => void
  undoRemoveTopSite: () => void
  setTopSitePosition: (url: string, pos: number) => void
}

export function defaultModel(): TopSitesModel {
  const state = defaultState()
  return {
    getState() { return state },
    addListener() { return () => {} },
    setShowTopSites(showTopSites) {},
    setListKind(listKind) {},
    addTopSite(url, title) {},
    updateTopSite(currentURL, newURL, title) {},
    removeTopSite(url) {},
    undoRemoveTopSite() {},
    setTopSitePosition(url, pos) {}
  }
}
