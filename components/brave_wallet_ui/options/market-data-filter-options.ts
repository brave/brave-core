import { AssetFilter } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AssetFilterOptions = (): AssetFilter[] => [
  {
    value: 'all',
    label: getLocale('braveWalletMarketDataAllAssetsFilter')
  },
  {
    value: 'watchlist',
    label: getLocale('braveWalletMarketDataWatchlistFilter')
  },
  {
    value: 'tradable',
    label: getLocale('braveWalletMarketDataTradableFilter')
  },
  {
    value: 'solana',
    label: getLocale('braveWalletMarketDataSolanaFilter')
  },
  {
    value: 'ethereum',
    label: getLocale('braveWalletMarketDataEthereumFilter')
  },
  {
    value: 'bsc',
    label: getLocale('braveWalletMarketDataBscFilter')
  }
]
