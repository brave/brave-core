import { MarketDataHeader } from '../components/market-datatable'

const alignRight = {
  textAlign: 'right'
}

// The id field matches values in MarketDataTableColumnTypes
// which match property fields in CoinMarketMetadata
// this helps in finding the correct header to sort by
export const MarketDataTableHeaders = [
  {
    id: 'assets',
    content: 'Assets'
  },
  {
    id: 'currentPrice',
    content: 'Price',
    sortable: true,
    customStyle: {
      ...alignRight
    }
  },
  {
    id: 'priceChange24h',
    content: '24hr',
    sortable: true,
    customStyle: {
      ...alignRight
    }
  },
  {
    id: 'marketCap',
    content: 'Mkt. Cap',
    sortable: true,
    sortOrder: 'desc',
    customStyle: {
      ...alignRight
    }
  },
  {
    id: 'totalVolume',
    content: 'Volume',
    sortable: true,
    customStyle: {
      ...alignRight
    }
  },
  {
    id: 'lineGraph',
    content: ''
  }
] as MarketDataHeader[]
