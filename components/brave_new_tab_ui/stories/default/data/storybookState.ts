import { select, boolean, number } from '@storybook/addon-knobs'
import { images } from '../../../data/backgrounds'
import { defaultTopSitesData } from '../../../data/defaultTopSites'
import dummyBrandedWallpaper from './brandedWallpaper'
import { defaultState } from '../../../storage/new_tab_storage'
import { initialGridSitesState } from '../../../storage/grid_sites_storage'

function generateStaticImages (images: NewTab.BackgroundWallpaper[]) {
  const staticImages = { SpaceX: undefined }
  for (const image of images) {
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

export const getNewTabData = (state: NewTab.State = defaultState): NewTab.State => ({
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
})

export const getGridSitesData = (
  state: NewTab.GridSitesState = initialGridSitesState
) => ({
  ...state,
  gridSites: generateTopSites(defaultTopSitesData)
})
