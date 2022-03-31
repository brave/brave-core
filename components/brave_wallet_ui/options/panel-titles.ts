import { PanelTitleObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const PanelTitles = (): PanelTitleObjectType[] => [
  {
    id: 'buy',
    title: getLocale('braveWalletBuy')
  },
  {
    id: 'send',
    title: getLocale('braveWalletSend')
  },
  {
    id: 'swap',
    title: getLocale('braveWalletSwap')
  },
  {
    id: 'apps',
    title: getLocale('braveWalletTopTabApps')
  },
  {
    id: 'sitePermissions',
    title: getLocale('braveWalletSitePermissionsTitle')
  },
  {
    id: 'transactions',
    title: getLocale('braveWalletRecentTransactions')
  },
  {
    id: 'transactionDetails',
    title: getLocale('braveWalletTransactionDetails')
  },
  {
    id: 'assets',
    title: getLocale('braveWalletAssetsPanelTitle')
  }
]
