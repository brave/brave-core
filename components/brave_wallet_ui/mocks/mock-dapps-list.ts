// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * UAW: unique active wallets
 */
export const mockDappsListMap = {
  'solana': {
    'success': true,
    'chain': 'solana',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 3182,
        'name': 'Rarible',
        'description': 'Today is the non-fungible reality of tomorrow.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/3182/rarible-dapp-marketplaces-eth-logo_6c8ebd4c788fc618a2cd1645c22950c9.png',
        'link': 'https://dappradar.com/dapp/rarible',
        'website': 'https://rarible.com',
        'chains': ['ethereum', 'flow', 'polygon', 'solana', 'tezos'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 11003626,
          'transactionsPercentageChange': -20.72,
          'uaw': 398971,
          'uawPercentageChange': 10.36,
          'volume': 74639.48,
          'volumePercentageChange': 44.37,
          'balance': 37.63,
          'balancePercentageChange': 66.87
        }
      },
      {
        'dappId': 10734,
        'name': 'Magic Eden',
        'description':
          'The NFT Marketplace Solana deserves, ' +
          'smooth as silk & fast as Solana. ' +
          '0% listing fee, only 2% transaction fee.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/10734/magiceden-dapp-marketplaces-solana-logo_4c3e2b8273f1b58465b6032f5dafd8ad.png',
        'link': 'https://dappradar.com/dapp/magic-eden',
        'website': 'https://magiceden.io/',
        'chains': ['solana'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 43780795,
          'transactionsPercentageChange': -5.59,
          'uaw': 64200,
          'uawPercentageChange': 2.15,
          'volume': 4136156.81,
          'volumePercentageChange': -36.83,
          'balance': 27478.42,
          'balancePercentageChange': -33.18
        }
      }
    ]
  },
  'ethereum': {
    'success': true,
    'chain': 'ethereum',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 21299,
        'name': 'Uniswap NFT Aggregator',
        'description':
          'Trade NFTs across major marketplaces ' +
          'to find more listings and better prices.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/21299/uniswapnftaggregator-dapp-marketplaces-ethereum-logo_96414b8e875f53c42e928d660f59eb00.png',
        'link': 'https://dappradar.com/dapp/uniswap-nft-aggregator',
        'website': 'https://app.uniswap.org/#/nfts',
        'chains': ['ethereum'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 908148,
          'transactionsPercentageChange': -73.52,
          'uaw': 193986,
          'uawPercentageChange': -59.73,
          'volume': 1663488110.48,
          'volumePercentageChange': -74.6,
          'balance': 0,
          'balancePercentageChange': 0
        }
      },
      {
        'dappId': 4096,
        'name': 'Uniswap V2',
        'description':
          'Uniswap is a fully decentralized protocol ' +
          'for automated liquidity provision on Ethereum.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/4096/uniswapv2-dapp-exchanges-ethereum-logo_11a4e3145c01c2cb04882e8c08654fbe.png',
        'link': 'https://dappradar.com/dapp/uniswap-v2',
        'website': 'https://app.uniswap.org/',
        'chains': ['ethereum'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 837384,
          'transactionsPercentageChange': -42.32,
          'uaw': 169343,
          'uawPercentageChange': -1.66,
          'volume': 10771629704.34,
          'volumePercentageChange': 445.92,
          'balance': 1038927752.97,
          'balancePercentageChange': 63.36
        }
      }
    ]
  }
}
