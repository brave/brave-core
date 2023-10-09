
// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { select, boolean, number, CHANGE } from '@storybook/addon-knobs'
import { addons } from '@storybook/addons'
import { defaultTopSitesData } from '../../../data/defaultTopSites'
import { defaultState } from '../../../storage/new_tab_storage'
import { initialGridSitesState } from '../../../storage/grid_sites_storage'
import { TabType as SettingsTabType } from '../../../containers/newTab/settings'
import dummyBrandedWallpaper from './brandedWallpaper'
import * as Background from './backgroundWallpaper'
import { newTabPrefManager } from '../../../hooks/usePref'
import { useEffect } from 'react'
import * as ColorUtil from '../../../helpers/colorUtil'

const addonsChannel = addons.getChannel()

function generateTopSites (topSites: typeof defaultTopSitesData) {
  const staticTopSites = []
  for (const [index, topSite] of topSites.entries()) {
    staticTopSites.push({
      ...topSite,
      title: topSite.name,
      letter: '',
      id: 'some-id-' + index,
      pinnedIndex: undefined,
      bookmarkInfo: undefined,
      defaultSRTopSite: false
    })
  }
  return staticTopSites
}

function shouldShowBrandedWallpaperData (shouldShow: boolean): NewTab.BrandedWallpaper | undefined {
  if (!shouldShow) {
    return undefined
  }
  return dummyBrandedWallpaper
}

function getWidgetStackOrder (firstWidget: string): NewTab.StackWidget[] {
  switch (firstWidget) {
    case 'braveTalk':
      return ['rewards', 'braveTalk']
    default:
      return ['braveTalk', 'rewards']
  }
}

/**
 * Guesses what the label for a settings key is. This is only accurate for the
 * case where the settings key is exactly a camelCased version of a
 * Sentence cased label ending with a '?'
 * i.e.
 * 'showStats' ==> 'Show stats?'
 *
 * A title case label or acronyms will break this, however, this isn't used
 * much at the moment.
 * @param key The key to guess the label for.
 * @returns The guessed label. Not guaranteed to be accurate, sorry :'(
 */
const guessLabelForKey = (key: string) => key
  .replace(/([A-Z])/g, (match) => ` ${match.toLowerCase()}`)
  .replace(/^./, (match) => match.toUpperCase())
  .trim() + '?'

export const useNewTabData = (state: NewTab.State = defaultState) => {
  const result: NewTab.State = {
    ...state,
    brandedWallpaper: shouldShowBrandedWallpaperData(
      boolean('Show branded background image?', true)
    ),
    brandedWallpaperOptIn: boolean('Show branded background image?', true),
    backgroundWallpaper: select(
      'Background',
      Background.backgroundWallpapers,
      Background.backgroundWallpapers.defaultImage
    ),
    readabilityThreshold: number('Readability threshold', ColorUtil.getThresholdForReadability(), { range: true, min: 0, max: 10, step: 0.1 }),
    customLinksEnabled: boolean('CustomLinks Enabled?', false),
    featureFlagBraveNTPSponsoredImagesWallpaper: true,
    featureCustomBackgroundEnabled: true,
    featureFlagBraveNewsPromptEnabled: true,
    featureFlagBraveNewsFeedV2Enabled: false,
    searchPromotionEnabled: false,
    forceSettingsTab: select('Open settings tab?', [undefined, ...Object.keys(SettingsTabType)], undefined),
    showBackgroundImage: boolean('Show background image?', true),
    showStats: boolean('Show stats?', true),
    showToday: boolean('Show Brave News?', true),
    showClock: boolean('Show clock?', true),
    clockFormat: select('Clock format?', ['', '12', '24'], ''),
    showTopSites: boolean('Show top sites?', true),
    braveRewardsSupported: boolean('Brave Rewards supported?', true),
    showRewards: boolean('Show rewards?', true),
    showBraveTalk: boolean('Show Brave Talk?', true),
    braveTalkSupported: boolean('Brave Talk supported?', true),
    hideAllWidgets: boolean('Hide all widgets?', false),
    isBraveNewsOptedIn: boolean('Brave News opted-in?', false),
    textDirection: select('Text direction', { ltr: 'ltr', rtl: 'rtl' }, 'ltr'),
    stats: {
      ...state.stats,
      adsBlockedStat: number('Number of blocked items', 1337),
      httpsUpgradesStat: number('Number of HTTPS upgrades', 1337)
    },
    initialDataLoaded: true,
    widgetStackOrder: getWidgetStackOrder(select('First widget', ['braveTalk', 'rewards'], 'rewards'))
  }

  // On all updates, notify that the prefs might've changed. Listeners are
  // only notified when the setting they're interested in updates anyway.
  useEffect(() => {
    newTabPrefManager.notifyListeners(result)
  })

  // In Storybook, we aren't wired up to the settings backend. This effect
  // **VERY** hackily overrides the savePref function to emit a knob change
  // event, guessing what the correct knob label is.

  // This is unfortunate, but the `addon-knob` library is deprecated and does
  // not offer any better way of setting knob values.

  // See https://github.com/storybookjs/storybook/issues/3855#issuecomment-624164453
  // for discussion of this issue.
  useEffect(() => {
    const originalSavePref = newTabPrefManager.savePref
    newTabPrefManager.savePref = (key, value) => {
      addonsChannel.emit(CHANGE, {
        name: guessLabelForKey(key),
        value: value
      })
    }
    return () => {
      newTabPrefManager.savePref = originalSavePref
    }
  }, [])

  return result
}

export const getGridSitesData = (
  state: NewTab.GridSitesState = initialGridSitesState
) => ({
  ...state,
  gridSites: generateTopSites(defaultTopSitesData)
})
