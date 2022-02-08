import { CoinMarketMetadata } from '../../constants/types'

export const mockCoinMarketData = [
  {
    coinGeckoID: 'bitcoin',
    symbol: 'BTC',
    name: 'Bitcoin',
    imageUrl: 'https://assets.coingecko.com/coins/images/1/large/bitcoin.png?1547033579',
    marketCap: 730938222066,
    marketCapRank: 1,
    currentPrice: 38484,
    priceChange24h: 1410.16,
    priceChangePercentage24h: 3.80361,
    totalVolume: 16642180918,
    priceHistory: [
        {
            date: 1643522508695,
            close: 38231.587452794774
        },
        {
            date: 1643526059250,
            close: 38141.19420739754
        },
        {
            date: 1643529608809,
            close: 38333.50041978721
        },
        {
            date: 1643652043565,
            close: 38557.619230601056
        }
    ]
  },
  {
    coinGeckoID: 'ethereum',
    symbol: 'ETH',
    name: 'Ethereum',
    imageUrl: 'https://assets.coingecko.com/coins/images/279/large/ethereum.png?1595348880',
    marketCap: 327123609655,
    marketCapRank: 2,
    currentPrice: 2733.56,
    priceChange24h: 221.7,
    priceChangePercentage24h: 8.82639,
    totalVolume: 13195027042,
    priceHistory: [
        {
            date: 1643522512834,
            close: 2608.164360156114
        },
        {
            date: 1643526022782,
            close: 2607.5346238439197
        },
        {
            date: 1643529651600,
            close: 2624.3120536430356
        },
        {
            date: 1643533312668,
            close: 2609.569606845281
        }
    ]
  },
  {
    coinGeckoID: 'tether',
    symbol: 'USDT',
    name: 'Tether',
    imageUrl: 'https://assets.coingecko.com/coins/images/325/large/Tether-logo.png?1598003707',
    marketCap: 78027219619,
    marketCapRank: 3,
    currentPrice: 0.999385,
    priceChange24h: -0.002154294966,
    priceChangePercentage24h: -0.2151,
    totalVolume: 36293608667,
    priceHistory: []
  },
  {
    coinGeckoID: 'binancecoin',
    symbol: 'BNB',
    name: 'Binance Coin',
    imageUrl: 'https://assets.coingecko.com/coins/images/825/large/binance-coin-logo.png?1547034615',
    marketCap: 64379401400,
    marketCapRank: 4,
    currentPrice: 381.73,
    priceChange24h: 11.41,
    priceChangePercentage24h: 3.08066,
    totalVolume: 1123893094,
    priceHistory: []
  },
  {
    coinGeckoID: 'usd-coin',
    symbol: 'USDC',
    name: 'USD Coin',
    imageUrl: 'https://assets.coingecko.com/coins/images/6319/large/USD_Coin_icon.png?1547042389',
    marketCap: 49965795004,
    marketCapRank: 5,
    currentPrice: 0.996943,
    priceChange24h: -0.004581891556,
    priceChangePercentage24h: -0.45749,
    totalVolume: 2551620106,
    priceHistory: []
  }
] as CoinMarketMetadata[]

export const fetchCoinMarketData = async (currencySymbols: string, perPage: number, page: number): Promise<CoinMarketMetadata[]> => {
  try {
    const url = `https://api.coingecko.com/api/v3/markets?vs_currency=${currencySymbols}&order=market_cap_desc&per_page=${perPage}&page=${page}`
    const response = await fetch(url)

    const { data } = await response.json()

    if (response.ok) {
      const coins = data.map((coin: any) => {
        // eslint-disable-next-line @typescript-eslint/naming-convention
        const { id, name, symbol, image, current_price, market_cap, market_cap_rank, price_change_percentage_24h_in_currency, price_change_percentage_24h, total_volume } = coin

        return {
          coinGeckoID: id,
          symbol,
          name,
          imageUrl: image.large,
          marketCap: market_cap,
          marketCapRank: market_cap_rank,
          currentPrice: current_price,
          priceChange24h: price_change_percentage_24h,
          priceChangePercentage24h: price_change_percentage_24h_in_currency,
          totalVolume: total_volume,
          priceHistory: []
        }
      })

      return coins
    } else {
      return Promise.reject(new Error(`${response.status} ${response.statusText}`))
    }
  } catch (error) {
    console.error(error)
    return Promise.reject(error)
  }
}
