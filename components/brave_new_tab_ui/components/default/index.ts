/* This Source Code Form is subject to the terms of the Mozilla Public
 * License,
 * v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { StatsContainer, StatsItem } from './stats'
import { Header, Main, Footer } from './grid'
import { SettingsMenu, SettingsRow, SettingsText, SettingsTitle, SettingsWrapper } from './settings'
import { List, ListWidget, Tile, TileActionsContainer, TileAction, TileFavicon } from './topSites'
import { SiteRemovalNotification, SiteRemovalText, SiteRemovalAction } from './notification'
import { ClockWidget } from './clock'
import createWidget from './widget'

export * from './page'

export {
  StatsContainer,
  StatsItem,
  Header,
  Main,
  Footer,
  List,
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
  createWidget
}
