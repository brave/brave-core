import { getLocale } from '../../common/locale'
import { MarketDataHeader } from '../components/market-datatable'

const alignRight = {
  textAlign: 'right'
}

// The id field matches values in MarketDataTableColumnTypes
// which match property fields in CoinMarketMetadata
// this helps in finding the correct header to sort by
export const marketDataTableHeaders = [
  {
    id: 'assets',
    content: getLocale('braveWalletMarketDataAssetsColumn'),
    customStyle: {
      width: 350,
      paddingLeft: 10
    }
  },
  {
    id: 'currentPrice',
    content: getLocale('braveWalletMarketDataPriceColumn'),
    sortable: true,
    customStyle: {
      ...alignRight,
      width: 150
    }
  },
  {
    id: 'priceChangePercentage24h',
    content: getLocale('braveWalletMarketData24HrColumn'),
    sortable: true,
    customStyle: {
      ...alignRight
    }
  },
  {
    id: 'marketCap',
    content: getLocale('braveWalletMarketDataMarketCapColumn'),
    sortable: true,
    sortOrder: 'desc',
    customStyle: {
      ...alignRight
    }
  },
  {
    id: 'totalVolume',
    content: getLocale('braveWalletMarketDataVolumeColumn'),
    sortable: true,
    customStyle: {
      ...alignRight
    }
  }
  // Hiden because price History data is not available
  // {
  //   id: 'lineGraph',
  //   content: ''
  // }
] as MarketDataHeader[]
