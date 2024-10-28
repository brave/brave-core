/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  NewTabState,
  NewTabActions,
  defaultNewTabState } from './new_tab'

import {
  BackgroundState,
  BackgroundActions,
  defaultBackgroundState } from './backgrounds'

import {
  RewardsState,
  RewardsActions,
  defaultRewardsState } from './rewards'

import {
  SearchState,
  SearchActions,
  defaultSearchState } from './search'

import {
  TopSitesState,
  TopSitesActions,
  defaultTopSitesState } from './top_sites'

import {
  VPNState,
  VPNActions,
  defaultVPNState } from './vpn'

export type AppState =
  NewTabState &
  BackgroundState &
  RewardsState &
  SearchState &
  TopSitesState &
  VPNState

export function defaultState(): AppState {
  return {
    ...defaultNewTabState(),
    ...defaultBackgroundState(),
    ...defaultRewardsState(),
    ...defaultSearchState(),
    ...defaultTopSitesState(),
    ...defaultVPNState()
  }
}

export type AppActions =
  NewTabActions &
  BackgroundActions &
  RewardsActions &
  SearchActions &
  TopSitesActions &
  VPNActions

export interface AppModel extends AppActions {
  getState: () => AppState
  addListener: (listener: (state: AppState) => void) => () => void
}
