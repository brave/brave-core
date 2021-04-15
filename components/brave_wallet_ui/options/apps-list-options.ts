import { AppsListType } from '../constants/types'
import locale from '../constants/locale'
const compoundIcon = require('../assets/app-icons/compound-icon.png')
const makerIcon = require('../assets/app-icons/maker-icon.jpeg')
const aaveIcon = require('../assets/app-icons/aave-icon.jpeg')
const openSeaIcon = require('../assets/app-icons/opensea-icon.png')
const raribleIcon = require('../assets/app-icons/rarible-icon.png')

export const AppsList: AppsListType[] = [
  {
    category: locale.defiCategory,
    categoryButtonText: locale.defiButtonText,
    appList: [
      {
        name: locale.compoundName,
        description: locale.compoundDescription,
        url: locale.compoundUrl,
        icon: compoundIcon
      },
      {
        name: locale.makerName,
        description: locale.makerDescription,
        url: locale.makerUrl,
        icon: makerIcon
      },
      {
        name: locale.aaveName,
        description: locale.aaveDescription,
        url: locale.aaveUrl,
        icon: aaveIcon
      }
    ]
  },
  {
    category: locale.nftCategory,
    categoryButtonText: locale.nftButtonText,
    appList: [
      {
        name: locale.openSeaName,
        description: locale.openSeaDescription,
        url: locale.openSeaUrl,
        icon: openSeaIcon
      },
      {
        name: locale.raribleName,
        description: locale.raribleDescription,
        url: locale.raribleUrl,
        icon: raribleIcon
      }
    ]
  }
]
