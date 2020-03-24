/* This Source Code Form is subject to the terms of the Mozilla Public
 * License,
 * v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { StatsContainer, StatsItem } from './stats'
import { SettingsMenu, SettingsRow, SettingsText, SettingsTitle, SettingsWrapper } from './settings'
import { ListWidget, Tile, TileActionsContainer, TileAction, TileFavicon } from './topSites'
import { SiteRemovalNotification, SiteRemovalText, SiteRemovalAction } from './notification'
import { ClockWidget } from './clock'
import RewardsWidget from './rewards'
import { WidgetStack } from './widgetStack'
import BinanceWidget from './binance'
import createWidget from './widget'

export * from './page'

export {
  StatsContainer,
  StatsItem,
  ListWidget,
  Tile,
  TileActionsContainer,
  TileAction,
  TileFavicon,
  SiteRemovalNotification,
  SiteRemovalText,
  SiteRemovalAction,
  ClockWidget,
  SettingsMenu,
  SettingsRow,
  SettingsText,
  SettingsTitle,
  SettingsWrapper,
  RewardsWidget,
  WidgetStack,
  BinanceWidget,
  createWidget
}
