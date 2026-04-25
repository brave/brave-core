/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { useBraveNews } from '../../../../components/brave_news/browser/resources/shared/Context'

import { useNewTabState } from '../context/new_tab_context'
import { useTopSitesState } from '../context/top_sites_context'
import { useRewardsState } from '../context/rewards_context'
import { useVpnState } from '../context/vpn_context'

// Returns a value indicating whether the search component is ready for layout
// based on application state initialization status.
export function useSearchLayoutReady() {
  const topSitesInitialized = useTopSitesState((s) => s.initialized)
  return topSitesInitialized
}

// Returns a value indicating whether the widget component is ready for layout
// based on application initialization status.
export function useWidgetLayoutReady() {
  const newTabInitialized = useNewTabState((s) => s.initialized)
  const rewardsInitialized = useRewardsState((s) => s.initialized)
  const vpnInitialized = useVpnState((s) => s.initialized)
  const newsInitialized = useBraveNewsInitialized()

  return (
    newTabInitialized && rewardsInitialized && vpnInitialized && newsInitialized
  )
}

function useBraveNewsInitialized() {
  const newsFeatureEnabled = useNewTabState((s) => s.newsFeatureEnabled)
  if (!newsFeatureEnabled) {
    return true
  }
  return useBraveNews().isShowOnNTPPrefEnabled !== undefined
}
