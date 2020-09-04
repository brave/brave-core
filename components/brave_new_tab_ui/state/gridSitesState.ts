// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
export function tilesUpdated (
  state: NewTab.GridSitesState,
  sitesData: NewTab.Site[]
): NewTab.GridSitesState {
  state = { ...state, gridSites: sitesData }
  return state
}

export function showTilesRemovedNotice (
  state: NewTab.GridSitesState,
  shouldShow: boolean
): NewTab.GridSitesState {
  state = { ...state, shouldShowSiteRemovedNotification: shouldShow }
  return state
}
