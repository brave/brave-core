// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  NEW_TAB_STATS_UPDATED = '@@newtab/NEW_TAB_STATS_UPDATED',
  NEW_TAB_PRIVATE_TAB_DATA_UPDATED = '@@newtab/NEW_TAB_PRIVATE_TAB_DATA_UPDATED',
  NEW_TAB_PREFERENCES_UPDATED = '@@newtab/NEW_TAB_PREFERENCES_UPDATED',
  NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION = '@@newtab/NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION',
  NEW_TAB_SET_INITIAL_DATA = '@@newtab/NEW_TAB_SET_INITIAL_DATA',
  REMOVE_STACK_WIDGET = '@@newtab/REMOVE_STACK_WIDGET',
  SET_FOREGROUND_STACK_WIDGET = '@@newtab/SET_FOREGROUND_STACK_WIDGET'
}

export type DismissBrandedWallpaperNotificationPayload = {
  isUserAction: boolean
}
