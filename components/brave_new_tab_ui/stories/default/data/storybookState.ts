import { select, boolean, number } from '@storybook/addon-knobs'
import { images } from '../../../data/backgrounds'
import { defaultTopSitesData } from '../../../data/defaultTopSites'
import dummyBrandedWallpaper from './brandedWallpaper'
import { defaultState } from '../../../storage/new_tab_storage'
import { initialGridSitesState } from '../../../storage/grid_sites_storage'

function generateStaticImages (images: NewTab.Image[]) {
  const staticImages = {}
  for (const image of images) {
    Object.assign(staticImages, {
      [image.author]: {
        ...image,
        source: require('../../../../img/newtab/backgrounds/' + image.source)
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

function shouldShowBrandedWallpaperData (shouldShow: boolean): NewTab.BrandedWallpaper {
  if (shouldShow === false) {
    return {
      wallpaperImageUrl: '',
      isSponsored: false,
      creativeInstanceId: '12345abcde',
      wallpaperId: 'abcde12345',
      logo: { image: '', companyName: '', alt: '', destinationUrl: '' }
    }
  }
  return dummyBrandedWallpaper
}

function getWidgetStackOrder (firstWidget: string): NewTab.StackWidget[] {
  switch (firstWidget) {
    case 'together':
      return ['rewards', 'binance', 'together', 'ftx']
    default:
      return ['together', 'binance', 'rewards', 'ftx']
  }
}

export const getNewTabData = (state: NewTab.State = defaultState): NewTab.State => ({
  ...state,
  brandedWallpaperData: shouldShowBrandedWallpaperData(
    boolean('Show branded background image?', true)
  ),
  backgroundImage: select(
    'Background image',
    generateStaticImages(images),
    generateStaticImages(images)['SpaceX']
  ),
  customLinksEnabled: boolean('CustomLinks Enabled?', false),
  showBackgroundImage: boolean('Show background image?', true),
  showStats: boolean('Show stats?', true),
  showToday: boolean('Show today?', true),
  showClock: boolean('Show clock?', true),
  showTopSites: boolean('Show top sites?', true),
  showRewards: boolean('Show rewards?', true),
  showTogether: boolean('Show together?', true),
  togetherSupported: boolean('Together supported?', true),
  togetherPromptDismissed: !boolean('Together prompt?', false),
  geminiSupported: boolean('Gemini Supported?', true),
  cryptoDotComSupported: boolean('Crypto.com supported?', true),
  ftxSupported: boolean('FTX supported?', true),
  showFTX: boolean('Show FTX?', true),
  showBinance: boolean('Show Binance?', true),
  isBraveTodayOptedIn: boolean('Brave Today opted-in?', false),
  textDirection: select('Text direction', { ltr: 'ltr', rtl: 'rtl' } , 'ltr'),
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
  widgetStackOrder: getWidgetStackOrder(select('First widget', ['together', 'rewards'], 'rewards'))
})

export const getGridSitesData = (
  state: NewTab.GridSitesState = initialGridSitesState
) => ({
  ...state,
  gridSites: generateTopSites(defaultTopSitesData)
})
