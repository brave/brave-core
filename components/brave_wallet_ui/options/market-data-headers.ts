// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import { MarketDataHeader } from '../components/market-datatable'

// The id field matches values in MarketDataTableColumnTypes
// which match property fields in CoinMarketMetadata
// this helps in finding the correct header to sort by
export const marketDataTableHeaders: MarketDataHeader[] = [
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
      width: 150,
      textAlign: 'right'
    }
  },
  {
    id: 'priceChangePercentage24h',
    content: getLocale('braveWalletMarketData24HrColumn'),
    sortable: true,
    customStyle: {
      textAlign: 'right'
    }
  },
  {
    id: 'marketCap',
    content: getLocale('braveWalletMarketDataMarketCapColumn'),
    sortable: true,
    sortOrder: 'desc',
    customStyle: {
      textAlign: 'right'
    }
  },
  {
    id: 'totalVolume',
    content: getLocale('braveWalletMarketDataVolumeColumn'),
    sortable: true,
    customStyle: {
      textAlign: 'right'
    }
  },
  {
    id: 'actions',
    content: 'Buy/Deposit',
    sortable: false,
    customStyle: {
      textAlign: 'right'
    }
  }
  // Hiden because price History data is not available
  // {
  //   id: 'lineGraph',
  //   content: ''
  // }
]
