import { select, boolean, number, CHANGE } from '@storybook/addon-knobs'
import { addons } from '@storybook/addons'
import { images } from '../../../data/backgrounds'
import { defaultTopSitesData } from '../../../data/defaultTopSites'
import { defaultState } from '../../../storage/new_tab_storage'
import { initialGridSitesState } from '../../../storage/grid_sites_storage'
import { TabType as SettingsTabType } from '../../../containers/newTab/settings'
import dummyBrandedWallpaper from './brandedWallpaper'
import { newTabPrefManager } from '../../../hooks/usePref'
import { useEffect } from 'react'

const addonsChannel = addons.getChannel()

function generateStaticImages (images: NewTab.BackgroundWallpaper[]) {
  const staticImages = { SpaceX: undefined }
  for (const image of images) {
    // author is optional field.
    if (!image.author) {
      continue
    }
    Object.assign(staticImages, {
      [image.author]: {
        ...image,
        wallpaperImageUrl: require('../../../../img/newtab/backgrounds/' + image.wallpaperImageUrl)
      }
    })
  }
  return staticImages
}

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
      return ['rewards', 'binance', 'braveTalk', 'ftx']
    default:
      return ['braveTalk', 'binance', 'rewards', 'ftx']
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
    backgroundWallpaper: select(
      'Background image',
      generateStaticImages(images),
      generateStaticImages(images).SpaceX
    ),
    customLinksEnabled: boolean('CustomLinks Enabled?', false),
    featureFlagBraveNewsEnabled: true,
    featureFlagBraveNewsPromptEnabled: true,
    searchPromotionEnabled: false,
    forceSettingsTab: select('Open settings tab?', [undefined, ...Object.keys(SettingsTabType)], undefined),
    showBackgroundImage: boolean('Show background image?', true),
    showStats: boolean('Show stats?', true),
    showToday: boolean('Show Brave News?', true),
    showClock: boolean('Show clock?', true),
    showTopSites: boolean('Show top sites?', true),
    showRewards: boolean('Show rewards?', true),
    showBraveTalk: boolean('Show Brave Talk?', true),
    braveTalkSupported: boolean('Brave Talk supported?', true),
    braveTalkPromptDismissed: !boolean('Brave Talk prompt?', false),
    geminiSupported: boolean('Gemini Supported?', true),
    cryptoDotComSupported: boolean('Crypto.com supported?', true),
    ftxSupported: boolean('FTX supported?', true),
    showFTX: boolean('Show FTX?', true),
    showBinance: boolean('Show Binance?', true),
    hideAllWidgets: boolean('Hide all widgets?', false),
    isBraveTodayOptedIn: boolean('Brave Today opted-in?', false),
    textDirection: select('Text direction', { ltr: 'ltr', rtl: 'rtl' }, 'ltr'),
    stats: {
      ...state.stats,
      adsBlockedStat: number('Number of blocked items', 1337),
      httpsUpgradesStat: number('Number of HTTPS upgrades', 1337)
    },
    // TODO(petemill): Support binance state when binance can be included without chrome.* APIs
    // binanceState: {
    //   ...state.binanceState,
    //   binanceSupported: boolean('Binance supported?', true)
    // },
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
