import { AssetFilter } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AssetFilterOptions = (): AssetFilter[] => [
  {
    value: 'all',
    label: getLocale('braveWalletMarketDataAllAssetsFilter')
  },
  {
    value: 'tradable',
    label: getLocale('braveWalletMarketDataTradableFilter')
  }
]
