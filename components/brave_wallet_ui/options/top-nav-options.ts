import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const TopNavOptions: TopTabNavObjectType[] = [
  {
    id: 'portfolio',
    name: locale.topNavPortfolio
  },
  {
    id: 'accounts',
    name: locale.topNavAccounts
  }
  // Temp commented out for MVP
  // {
  //   id: 'apps',
  //   name: locale.topTabApps
  // }
]
