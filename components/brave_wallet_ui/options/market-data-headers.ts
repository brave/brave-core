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
    content: 'Assets',
    customStyle: {
      width: 350,
      paddingLeft: 10
    }
  },
  {
    id: 'currentPrice',
    content: 'Price',
    sortable: true,
    customStyle: {
      ...alignRight,
      width: 150
    }
  },
  {
    id: 'priceChangePercentage24h',
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
  }
  // Hiden because price History data is not available
  // {
  //   id: 'lineGraph',
  //   content: ''
  // }
] as MarketDataHeader[]
