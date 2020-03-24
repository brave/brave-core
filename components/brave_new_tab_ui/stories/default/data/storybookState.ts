import { select, boolean, number } from '@storybook/addon-knobs/react'
import { images } from '../../../data/backgrounds'
import { defaultTopSitesData } from '../../../data/defaultTopSites'
import dummyBrandedWallpaper from './brandedWallpaper'

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
      index,
      title: topSite.name,
      letter: '',
      thumb: '',
      themeColor: '',
      computedThemeColor: '',
      pinned: false
    })
  }
  return staticTopSites
}

function shouldShowBrandedWallpaperData (shouldShow: boolean) {
  if (shouldShow === false) {
    return {
      BrandedWallpaper: '',
      wallpaperImageUrl: '',
      logo: { image: '', companyName: '', alt: '', destinationUrl: '' }
    }
  }
  return dummyBrandedWallpaper
}

export const getNewTabData = (state: NewTab.State) => ({
  ...state,
  brandedWallpaperData: shouldShowBrandedWallpaperData(boolean('Show branded background image?', true)),
  backgroundImage: select('Background image', generateStaticImages(images), generateStaticImages(images)['SpaceX']),
  showBackgroundImage: boolean('Show background image?', true),
  showStats: boolean('Show stats?', true),
  showClock: boolean('Show clock?', true),
  showTopSites: boolean('Show top sites?', true),
  showRewards: boolean('Show rewards?', true),
  showBinance: boolean('Show Binance?', true),
  textDirection: select('Text direction', { ltr: 'ltr', rtl: 'rtl' } , 'ltr'),
  gridSites: generateTopSites(defaultTopSitesData),
  stats: {
    ...state.stats,
    adsBlockedStat: number('Number of blocked items', 1337),
    httpsUpgradesStat: number('Number of HTTPS upgrades', 1337)
  },
  initialDataLoaded: true,
  currentStackWidget: 'rewards' as NewTab.StackWidget
})
