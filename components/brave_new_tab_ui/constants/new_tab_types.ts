// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as newTabActions from '../actions/new_tab_actions'
import * as gridSitesActions from '../actions/grid_sites_actions'
import * as binanceActions from '../actions/binance_actions'
import * as rewardsActions from '../actions/rewards_actions'
import * as geminiActions from '../actions/gemini_actions'
import * as cryptoDotComActions from '../actions/cryptoDotCom_actions'
import * as ftxActions from '../widgets/ftx/ftx_actions'
import * as stackWidgetActions from '../actions/stack_widget_actions'
import * as todayActions from '../actions/today_actions'

export const enum types {
  NEW_TAB_STATS_UPDATED = '@@newtab/NEW_TAB_STATS_UPDATED',
  NEW_TAB_PRIVATE_TAB_DATA_UPDATED = '@@newtab/NEW_TAB_PRIVATE_TAB_DATA_UPDATED',
  NEW_TAB_TOR_TAB_DATA_UPDATED = '@@newtab/NEW_TAB_TOR_TAB_DATA_UPDATED',
  NEW_TAB_PREFERENCES_UPDATED = '@@newtab/NEW_TAB_PREFERENCES_UPDATED',
  NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION = '@@newtab/NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION',
  NEW_TAB_SET_INITIAL_DATA = '@@newtab/NEW_TAB_SET_INITIAL_DATA',
  SET_MOST_VISITED_SITES = '@@newtab/SET_MOST_VISITED_SITES',
  TOP_SITES_STATE_UPDATED = '@@newtab/TOP_SITES_STATE_UPDATED',
  CUSTOMIZE_CLICKED = '@@newtab/CUSTOMIZE_CLICKED',
  UPDATE_CLOCK_WIDGET = '@@newtab/UPDATE_CLOCK_WIDGET'
}

export type DismissBrandedWallpaperNotificationPayload = {
  isUserAction: boolean
}

export type NewTabActions =
  typeof newTabActions &
  typeof gridSitesActions &
  typeof binanceActions &
  typeof rewardsActions &
  typeof geminiActions &
  typeof cryptoDotComActions &
  typeof stackWidgetActions &
  {
    today: typeof todayActions,
    ftx: typeof ftxActions
  }
