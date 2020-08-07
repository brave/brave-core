import { select, boolean, number } from '@storybook/addon-knobs/react'
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

function shouldShowBrandedWallpaperData (shouldShow: boolean) {
  if (shouldShow === false) {
    return {
      wallpaperImageUrl: '',
      isSponsored: false,
      logo: { image: '', companyName: '', alt: '', destinationUrl: '' }
    }
  }
  return dummyBrandedWallpaper
}

function getWidgetStackOrder (firstWidget: string): NewTab.StackWidget[] {
  switch (firstWidget) {
    case 'together':
      return ['rewards', 'binance', 'together']
    default:
      return ['together', 'binance', 'rewards']
  }
}

export const getNewTabData = (state: NewTab.State = defaultState) => ({
  ...state,
  brandedWallpaperData: shouldShowBrandedWallpaperData(
    boolean('Show branded background image?', true)
  ),
  backgroundImage: select(
    'Background image',
    generateStaticImages(images),
    generateStaticImages(images)['SpaceX']
  ),
  showBackgroundImage: boolean('Show background image?', true),
  showStats: boolean('Show stats?', true),
  showClock: boolean('Show clock?', true),
  showTopSites: boolean('Show top sites?', true),
  showRewards: boolean('Show rewards?', true),
  showTogether: boolean('Show together?', true),
  togetherSupported: boolean('Together supported?', true),
  geminiSupported: boolean('Gemini Supported?', true),
  showBinance: boolean('Show Binance?', true),
  showAddCard: true,
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
