/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StatsContainer, StatsItem } from './stats'
export * from './settings'
import { AddSiteTile, Tile, TileActionsContainer, TileAction, TileImageContainer, TileFavicon, TileMenu, TileMenuItem, TileTitle } from './gridSites'
import { SiteRemovalNotification, SiteRemovalText, SiteRemovalAction } from './notification'
import { Clock } from './clock'
import { RewardsWidget } from './rewards'
import { VPNWidget } from './vpn'
import { BraveTalkWidget } from './braveTalk'
import EditTopSite from './editTopSite'
import SearchPromotion from './searchPromotion'
import createWidget from './widget'

export * from './page'

export {
  StatsContainer,
  StatsItem,
  AddSiteTile,
  Tile,
  TileActionsContainer,
  TileAction,
  TileImageContainer,
  TileFavicon,
  TileMenu,
  TileMenuItem,
  TileTitle,
  SiteRemovalNotification,
  SiteRemovalText,
  SiteRemovalAction,
  Clock,
  RewardsWidget,
  BraveTalkWidget,
  VPNWidget,
  createWidget,
  EditTopSite,
  SearchPromotion
}
