// Copyright (c) 2023 The Brave Authors. All rights reserved.
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
          'The NFT Marketplace Solana deserves, smooth as silk & fast as Solana. 0% listing fee, only 2% transaction fee.',
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
      },
      {
        'dappId': 13812,
        'name': 'MeanFi',
        'description':
          'Self-custody crypto treasury management with real-time payment streaming. No-code token vesting, crypto payments and multisig on Solana.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/13812/meanfi-dapp-defi-solana-logo_fe398c8642606b723bbbcdb3181d4784.png',
        'link': 'https://dappradar.com/dapp/meanfi',
        'website': 'https://meanfi.com/',
        'chains': ['solana'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 232657,
          'transactionsPercentageChange': 31.22,
          'uaw': 54321,
          'uawPercentageChange': 32.56,
          'volume': 4433.43,
          'volumePercentageChange': 20208.81,
          'balance': 822.48,
          'balancePercentageChange': 73.19
        }
      },
      {
        'dappId': 27755,
        'name': 'Sol Incinerator',
        'description': 'Burn any unwanted NFTs or tokens and reclaim the rent',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/27755/solincinerator-dapp-other-solana-logo_333f394674a5c1f490cb1cddaff52e6f.png',
        'link': 'https://dappradar.com/dapp/sol-incinerator',
        'website': 'https://sol-incinerator.com/#/',
        'chains': ['solana'],
        'categories': ['other'],
        'metrics': {
          'transactions': 144121,
          'transactionsPercentageChange': 2.45,
          'uaw': 20180,
          'uawPercentageChange': 32.97,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': 0.05,
          'balancePercentageChange': 66.67
        }
      },
      {
        'dappId': 12972,
        'name': 'Star Atlas',
        'description':
          'Virtual gaming metaverse on the Solana blockchain with Unreal Engine 5 real-time graphics, multiplayer game, and decentralized financial technologies.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/12972/staratlas-dapp-games-solana-logo_705568fabf4a846347ac34665a7a3293.png',
        'link': 'https://dappradar.com/dapp/star-atlas',
        'website': 'https://staratlas.com',
        'chains': ['solana'],
        'categories': ['games'],
        'metrics': {
          'transactions': 53083322,
          'transactionsPercentageChange': 453.73,
          'uaw': 16417,
          'uawPercentageChange': 11.29,
          'volume': 66937.94,
          'volumePercentageChange': 114.7,
          'balance': 2341.17,
          'balancePercentageChange': 68.2
        }
      },
      {
        'dappId': 10231,
        'name': 'Raydium',
        'description':
          'An on-chain order book AMM powering the evolution of DeFi',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/10231/raydium-dapp-defi-solana-logo_17fc396d07351830764dbb69ddb495f2.png',
        'link': 'https://dappradar.com/dapp/raydium',
        'website': 'https://raydium.io/',
        'chains': ['solana'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 956782,
          'transactionsPercentageChange': 42.48,
          'uaw': 14893,
          'uawPercentageChange': 69.95,
          'volume': 345621.85,
          'volumePercentageChange': 160.71,
          'balance': 172827.47,
          'balancePercentageChange': 7.1
        }
      },
      {
        'dappId': 10267,
        'name': 'Saber',
        'description':
          'Saber is a protocol enabling seamless cross-chain liquidity exchange.\nSupporting assets from Solana, Ethereum, BSC, Polygon, Celo, and more.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/10267/saber-dapp-defi-solana-logo_f5b15f9a0a534adaee61be10fc97d6c6.png',
        'link': 'https://dappradar.com/dapp/saber',
        'website': 'https://saber.so/',
        'chains': ['solana'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 2732,
          'transactionsPercentageChange': -23.84,
          'uaw': 11901,
          'uawPercentageChange': -10.1,
          'volume': 62786456.2,
          'volumePercentageChange': -14.93,
          'balance': 8027852.9,
          'balancePercentageChange': 16.67
        }
      },
      {
        'dappId': 20399,
        'name': 'Chingari',
        'description':
          'Chingari & GARI Network is the leading short video app & web3 social network. Users receive incentives in the form of GARI Tokens for their contributions.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20399/chingari-dapp-social-solana-logo_e9f36db4b6ef82d1a46910fe4fe51366.png',
        'link': 'https://dappradar.com/dapp/chingari',
        'website': 'https://linktr.ee/garinetwork',
        'chains': ['other', 'solana'],
        'categories': ['social'],
        'metrics': {
          'transactions': 42594,
          'transactionsPercentageChange': -15.01,
          'uaw': 7649,
          'uawPercentageChange': -26.82,
          'volume': 69316.27,
          'volumePercentageChange': 1.75,
          'balance': 221560.45,
          'balancePercentageChange': -27.16
        }
      },
      {
        'dappId': 15123,
        'name': 'Genopets',
        'description': "The world's first Move-to-Earn NFT Game.",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/15123/genopets-dapp-games-15123-logo_1b50ed342106b1e23035b632dd017fde.png',
        'link': 'https://dappradar.com/dapp/genopets',
        'website': 'https://linktr.ee/genopets',
        'chains': ['binance-smart-chain', 'solana'],
        'categories': ['games', 'collectibles'],
        'metrics': {
          'transactions': 316931,
          'transactionsPercentageChange': -5.27,
          'uaw': 6934,
          'uawPercentageChange': -9.34,
          'volume': 34420.32,
          'volumePercentageChange': 32.33,
          'balance': 272.87,
          'balancePercentageChange': 73.94
        }
      },
      {
        'dappId': 40617,
        'name': 'Access Protocol',
        'description':
          'Application for content creator monetization and consumption',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/40617/accessprotocol-dapp-social-40617-logo_0aac75b9be9e365e7aeb1738de9c8c87.png',
        'link': 'https://dappradar.com/dapp/access-protocol',
        'website': 'https://hub.accessprotocol.co/creators',
        'chains': ['solana'],
        'categories': ['social'],
        'metrics': {
          'transactions': 65961,
          'transactionsPercentageChange': -36.26,
          'uaw': 6597,
          'uawPercentageChange': -13.36,
          'volume': 143.99,
          'volumePercentageChange': -71.34,
          'balance': 0.05,
          'balancePercentageChange': 66.67
        }
      },
      {
        'dappId': 17400,
        'name': 'Solcasino.io',
        'description': 'Top One Crypto Casino on Solana network.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/17400/solcasinoio-dapp-gambling-solana-logo_b9355f342ba773673051527e3ec1ad9a.png',
        'link': 'https://dappradar.com/dapp/solcasino-io',
        'website': 'https://solcasino.io',
        'chains': ['solana'],
        'categories': ['gambling'],
        'metrics': {
          'transactions': 38129,
          'transactionsPercentageChange': 13.19,
          'uaw': 6444,
          'uawPercentageChange': 35.32,
          'volume': 2700396.9,
          'volumePercentageChange': 1.58,
          'balance': 44209.55,
          'balancePercentageChange': -88.94
        }
      },
      {
        'dappId': 15720,
        'name': 'OwlDAO',
        'description':
          'OwlDAO is the leading blockchain-based gaming provider for DeFi Projects. The DAO helps projects build their own Casinos to GameFi their tokens.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/15720/owldao-dapp-gambling-matic-logo_b85049d2f8c14c45bd2af099b129dcc7.png',
        'link': 'https://dappradar.com/dapp/owldao',
        'website': 'https://owldao.io/',
        'chains': [
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'cronos',
          'ethereum',
          'fantom',
          'moonriver',
          'near',
          'polygon',
          'solana'
        ],
        'categories': ['gambling'],
        'metrics': {
          'transactions': 37399,
          'transactionsPercentageChange': 11.05,
          'uaw': 6438,
          'uawPercentageChange': 35.34,
          'volume': 2608099.77,
          'volumePercentageChange': -1.89,
          'balance': 0.05,
          'balancePercentageChange': 66.67
        }
      },
      {
        'dappId': 26626,
        'name': 'Sharky',
        'description':
          '#1 escrowless NFT lending and borrowing protocol on Solana. Borrow SOL against your NFTs, Lend SOL and earn a high % APY.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/26626/sharky-dapp-defi-solana-logo_732a78c3c53e49af4863245005e4f769.png',
        'link': 'https://dappradar.com/dapp/sharky',
        'website': 'https://sharky.fi',
        'chains': ['solana'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 3553987,
          'transactionsPercentageChange': 41.89,
          'uaw': 4975,
          'uawPercentageChange': -8.56,
          'volume': 368649069.93,
          'volumePercentageChange': 99.21,
          'balance': 0.05,
          'balancePercentageChange': 66.67
        }
      },
      {
        'dappId': 10187,
        'name': 'Mercurial',
        'description': 'Building the Liquidity Platform for Stables on Solana',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/10187/mercurial-dapp-defi-solana-logo_3dfc12808771cf53aa6f9d157f4f8205.png',
        'link': 'https://dappradar.com/dapp/mercurial',
        'website': 'https://mercurial.finance',
        'chains': ['solana'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 252,
          'transactionsPercentageChange': 375.47,
          'uaw': 3640,
          'uawPercentageChange': 310.37,
          'volume': 13439390.55,
          'volumePercentageChange': 242.52,
          'balance': 151952.6,
          'balancePercentageChange': -97.08
        }
      },
      {
        'dappId': 14696,
        'name': 'Tap Fantasy',
        'description':
          'Tap Fantasy is an MMORPG building the biggest play2earn on Solana and BSC. It’s the metaverse version of the famous TapTap Fantasy with over 20 million users.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/14696/tapfantasy-dapp-games-bsc-logo_249ad805b7d64c63971496328a42a44b.png',
        'link': 'https://dappradar.com/dapp/tap-fantasy',
        'website': 'https://tapfantasy.io/',
        'chains': ['binance-smart-chain', 'solana'],
        'categories': ['games'],
        'metrics': {
          'transactions': 30218,
          'transactionsPercentageChange': 1.01,
          'uaw': 3559,
          'uawPercentageChange': 0.03,
          'volume': 0,
          'volumePercentageChange': -100,
          'balance': 212.08,
          'balancePercentageChange': 58.77
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
          'Trade NFTs across major marketplaces to find more listings and better prices.',
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
          'Uniswap is a fully decentralized protocol for automated liquidity provision on Ethereum.',
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
      },
      {
        'dappId': 7000,
        'name': 'Uniswap V3',
        'description':
          'A protocol for trading and automated liquidity provision on Ethereum.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7000/uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png',
        'link': 'https://dappradar.com/dapp/uniswap-v3',
        'website': 'https://app.uniswap.org/#/swap',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 190660,
          'transactionsPercentageChange': 32.6,
          'uaw': 130937,
          'uawPercentageChange': 16.45,
          'volume': 60290701064.9,
          'volumePercentageChange': 5.48,
          'balance': 1659836306.81,
          'balancePercentageChange': -0.36
        }
      },
      {
        'dappId': 10215,
        'name': 'MetaMask Swap',
        'description':
          'A crypto swap from the wallet & gateway to blockchain apps. Trusted by over 5 million users worldwide.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/10215/metamaskswap-dapp-defi-ethereum-logo_2e2277f0d2a3604efab41b3693d4e398.png',
        'link': 'https://dappradar.com/dapp/metamask-swap',
        'website': 'https://metamask.io/',
        'chains': ['ethereum'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 225447,
          'transactionsPercentageChange': 26.04,
          'uaw': 90354,
          'uawPercentageChange': 15.08,
          'volume': 107329473.31,
          'volumePercentageChange': 37.37,
          'balance': 4598966.87,
          'balancePercentageChange': 12.54
        }
      },
      {
        'dappId': 13,
        'name': 'OpenSea',
        'description':
          'OpenSea is the first and largest peer-to-peer marketplace.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/13/opensea-dapp-marketplaces-ethereum-logo_a3421ac6e32d529db3c8293f4cfa9bcd.png',
        'link': 'https://dappradar.com/dapp/opensea',
        'website': 'https://opensea.io',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'klaytn',
          'optimism',
          'polygon',
          'solana'
        ],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 247560,
          'transactionsPercentageChange': 0.78,
          'uaw': 75590,
          'uawPercentageChange': -2.75,
          'volume': 49482336.26,
          'volumePercentageChange': 8.83,
          'balance': 65712.87,
          'balancePercentageChange': 9.15
        }
      },
      {
        'dappId': 257,
        'name': '0x Protocol',
        'description':
          '0x is an open protocol that enables the peer-to-peer exchange of assets on the Ethereum blockchain.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/257/0xprotocol-dapp-other-eth-logo_7e869e1e5fbe2b41499661ea503195bd.png',
        'link': 'https://dappradar.com/dapp/0x-protocol',
        'website': 'https://www.0x.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 183138,
          'transactionsPercentageChange': 32.16,
          'uaw': 60691,
          'uawPercentageChange': 20.27,
          'volume': 759566724.42,
          'volumePercentageChange': 15.85,
          'balance': 20180617.01,
          'balancePercentageChange': 40.86
        }
      },
      {
        'dappId': 1528,
        'name': '1inch Network',
        'description':
          'A distributed DeFi aggregator for various protocols on multiple chains.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/1528/1inchexchange-dapp-exchanges-ethereum-logo_855963bbf18d4c089e92160b47b43342.png',
        'link': 'https://dappradar.com/dapp/1inch-network',
        'website': 'https://app.1inch.io',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'klaytn',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 249495,
          'transactionsPercentageChange': 12.9,
          'uaw': 59116,
          'uawPercentageChange': 15,
          'volume': 3915347861.32,
          'volumePercentageChange': 42.51,
          'balance': 31783.15,
          'balancePercentageChange': 10.72
        }
      },
      {
        'dappId': 20645,
        'name': 'Blur',
        'description': 'The NFT marketplace for pro traders.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20645/blur-dapp-marketplaces-ethereum-logo_567ad31ff357961f9faf589237bae217.png',
        'link': 'https://dappradar.com/dapp/blur',
        'website': 'https://blur.io/collections',
        'chains': ['ethereum'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 158883,
          'transactionsPercentageChange': 12.95,
          'uaw': 39859,
          'uawPercentageChange': 12.07,
          'volume': 219735298.51,
          'volumePercentageChange': 22.01,
          'balance': 67408097.34,
          'balancePercentageChange': 0.73
        }
      },
      {
        'dappId': 43969,
        'name': 'Move Stake',
        'description':
          'Move Stake is a Big Multi Chain Decentralized Staking Yield\n1% - 4% Basic Daily ROI\n23% Referral Rewards in 4 Levels\nfree MVB token and many more',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/43969/movestake-dapp-high-risk-43969-logo_3d820705633cc75d8fe0f187aabc4dfa.png',
        'link': 'https://dappradar.com/dapp/move-stake',
        'website': 'https://movestake.io/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['high-risk'],
        'metrics': {
          'transactions': 53616,
          'transactionsPercentageChange': -17.4,
          'uaw': 37910,
          'uawPercentageChange': -10.24,
          'volume': 72440.46,
          'volumePercentageChange': -3.29,
          'balance': 4113.23,
          'balancePercentageChange': -61.1
        }
      },
      {
        'dappId': 18305,
        'name': 'Stargate',
        'description': 'A Composable Omnichain Native Asset Bridge',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png',
        'link': 'https://dappradar.com/dapp/stargate',
        'website': 'https://stargate.finance/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 25085,
          'transactionsPercentageChange': -27.6,
          'uaw': 18822,
          'uawPercentageChange': -30.28,
          'volume': 216489549.76,
          'volumePercentageChange': 9.97,
          'balance': 165492846.97,
          'balancePercentageChange': 4.78
        }
      },
      {
        'dappId': 25959,
        'name': 'Manifold',
        'description': 'Enabling creative sovereignty in web3',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/25959/manifold-dapp-marketplaces-ethereum-logo_47712d1e78deb9d462fa4f012cb750f3.png',
        'link': 'https://dappradar.com/dapp/manifold',
        'website': 'https://studio.manifold.xyz/',
        'chains': ['ethereum', 'optimism'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 31975,
          'transactionsPercentageChange': -54.36,
          'uaw': 14797,
          'uawPercentageChange': -74.17,
          'volume': 2067245.13,
          'volumePercentageChange': -39.89,
          'balance': 1086559.36,
          'balancePercentageChange': 14.91
        }
      },
      {
        'dappId': 7824,
        'name': 'LIDO',
        'description':
          "The Lido Ethereum Liquid Staking Protocol, built on Ethereum 2.0's Beacon chain, allows their users to earn staking rewards on the Beacon chain without locking",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7824/lido-dapp-defi-ethereum-logo_c449fb18f54140d3f0edfe29195c35bb.png',
        'link': 'https://dappradar.com/dapp/lido',
        'website': 'https://www.lido.fi',
        'chains': ['ethereum', 'moonbeam', 'moonriver'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 16671,
          'transactionsPercentageChange': 9.43,
          'uaw': 12982,
          'uawPercentageChange': 9.7,
          'volume': 606733012.9,
          'volumePercentageChange': 42.73,
          'balance': 302614114.93,
          'balancePercentageChange': 18.04
        }
      },
      {
        'dappId': 23657,
        'name': 'OKX DEX',
        'description':
          'OKX DEX optimizes your output by finding the best Liquidity Provider (LP) and the best route that minimizes slippage and network fees.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/23657/okxdex-dapp-exchanges-ethereum-logo_5cd32f9b140ffc6957c27fae4f3dcd85.png',
        'link': 'https://dappradar.com/dapp/okx-dex',
        'website':
          'https://www.okx.com/web3/dex?inputChain=1&inputCurrency=ETH&outputChain=1&outputCurrency=0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'cronos',
          'ethereum',
          'fantom',
          'optimism',
          'polygon',
          'solana',
          'tron'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 51643,
          'transactionsPercentageChange': 38.05,
          'uaw': 12029,
          'uawPercentageChange': 33.58,
          'volume': 51544662.83,
          'volumePercentageChange': 91.35,
          'balance': 0.01,
          'balancePercentageChange': -100
        }
      },
      {
        'dappId': 44959,
        'name': 'Layerswap',
        'description':
          'Fast and reliable crypto transfers across networks and exchanges.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/44959/layerswap-dapp-defi-44959-logo_b89e707bd7b84f4852158beb246ec33c.png',
        'link': 'https://dappradar.com/dapp/layerswap',
        'website': 'https://www.layerswap.io/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'immutablex',
          'optimism',
          'other',
          'polygon',
          'solana',
          'zksync-era'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 13541,
          'transactionsPercentageChange': 24.39,
          'uaw': 11820,
          'uawPercentageChange': 27.62,
          'volume': 9257387.31,
          'volumePercentageChange': 4.87,
          'balance': 627359.32,
          'balancePercentageChange': 83.16
        }
      },
      {
        'dappId': 6728,
        'name': 'CoW Swap',
        'description':
          'CoW Protocol finds the lowest price for your trade across all exchanges and aggregators, such as Uniswap and 1inch and protects you from MEV, unlike the others.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/6728/cowswap-dapp-defi-ethereum-logo_f454c122e07a03fc17547488ecf789ef.png',
        'link': 'https://dappradar.com/dapp/cow-swap',
        'website': 'https://cowswap.exchange',
        'chains': ['ethereum', 'other'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 27582,
          'transactionsPercentageChange': 34.03,
          'uaw': 11472,
          'uawPercentageChange': 32.61,
          'volume': 2023784412.46,
          'volumePercentageChange': 30.48,
          'balance': 231944.72,
          'balancePercentageChange': 53.18
        }
      }
    ]
  },
  'polygon': {
    'success': true,
    'chain': 'polygon',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 20598,
        'name': 'Galxe',
        'description': 'The largest Web3 credential data network in the world.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20598/galxe-dapp-social-matic-logo_033b8027624b1a20f188c1e991201392.png',
        'link': 'https://dappradar.com/dapp/galxe',
        'website': 'https://galxe.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'iotex',
          'moonbeam',
          'optimism',
          'polygon'
        ],
        'categories': ['social'],
        'metrics': {
          'transactions': 1702836,
          'transactionsPercentageChange': -32.54,
          'uaw': 447028,
          'uawPercentageChange': -17.76,
          'volume': 14880.51,
          'volumePercentageChange': -30.66,
          'balance': 171.86,
          'balancePercentageChange': 14.78
        }
      },
      {
        'dappId': 37892,
        'name': 'Kratos Studios',
        'description':
          'Kratos Studios is aggregating gaming communities across emerging markets & is building on-chain reputation system for gamers & gaming clans. Kratos owns IndiGG.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/37892/kratosstudios-dapp-games-37892-logo_712fb5843dcd2d852e8781df06cfedaa.png',
        'link': 'https://dappradar.com/dapp/kratos-studios',
        'website': 'https://kratos.global',
        'chains': ['optimism', 'polygon'],
        'categories': ['games', 'collectibles', 'marketplaces'],
        'metrics': {
          'transactions': 87624,
          'transactionsPercentageChange': -66.71,
          'uaw': 231375,
          'uawPercentageChange': 39.73,
          'volume': 76918.08,
          'volumePercentageChange': 18.23,
          'balance': 283.19,
          'balancePercentageChange': -72.42
        }
      },
      {
        'dappId': 18305,
        'name': 'Stargate',
        'description': 'A Composable Omnichain Native Asset Bridge',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png',
        'link': 'https://dappradar.com/dapp/stargate',
        'website': 'https://stargate.finance/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 229432,
          'transactionsPercentageChange': -36.02,
          'uaw': 140572,
          'uawPercentageChange': -25.45,
          'volume': 92017638.72,
          'volumePercentageChange': -28.2,
          'balance': 29884529.41,
          'balancePercentageChange': -11.84
        }
      },
      {
        'dappId': 44115,
        'name': 'GOQii',
        'description':
          'Empower you to prioritize your well-being and make preventive healthcare a rewarding part of your life',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/44115/goqii-dapp-other-44115-logo_789a206896edfe1692e09d8f5b915250.png',
        'link': 'https://dappradar.com/dapp/goqii',
        'website': 'https://universalhealthtoken.xyz',
        'chains': ['polygon'],
        'categories': ['other'],
        'metrics': {
          'transactions': 1245652,
          'transactionsPercentageChange': 280.34,
          'uaw': 133624,
          'uawPercentageChange': 4.26,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 7000,
        'name': 'Uniswap V3',
        'description':
          'A protocol for trading and automated liquidity provision on Ethereum.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7000/uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png',
        'link': 'https://dappradar.com/dapp/uniswap-v3',
        'website': 'https://app.uniswap.org/#/swap',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 510204,
          'transactionsPercentageChange': 84.81,
          'uaw': 88472,
          'uawPercentageChange': 8.35,
          'volume': 2172644834.77,
          'volumePercentageChange': 44.08,
          'balance': 65767614.83,
          'balancePercentageChange': -0.73
        }
      },
      {
        'dappId': 37067,
        'name': 'QORPO WORLD',
        'description':
          'QORPO WORLD takes you closer to Web3, games, esports, and authentic ownership. Own your game with $QORPO. IP branded titles AneeMate & Citizen Conflict.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/37067/qorpoworld-dapp-games-37067-logo_f2d754889099c318f08b6032b5ff7c87.png',
        'link': 'https://dappradar.com/dapp/qorpo-world',
        'website': 'https://qorpo.world/',
        'chains': ['binance-smart-chain', 'polygon'],
        'categories': ['games', 'collectibles', 'marketplaces'],
        'metrics': {
          'transactions': 294366,
          'transactionsPercentageChange': 790.4,
          'uaw': 68603,
          'uawPercentageChange': 3079.01,
          'volume': 205779.15,
          'volumePercentageChange': 283.61,
          'balance': 2.33,
          'balancePercentageChange': 20.73
        }
      },
      {
        'dappId': 17573,
        'name': 'Jump.trade',
        'description':
          'Jump.trade, a unique NFT platform, where you can experience one-of-kind  NFTs featuring game, brands and global artists.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/17573/jumptrade-dapp-marketplaces-matic-logo_3ff6123ad848b677975cfb94637f06cc.png',
        'link': 'https://dappradar.com/dapp/jump-trade',
        'website': 'https://www.jump.trade/',
        'chains': ['near', 'polygon', 'skale'],
        'categories': ['games', 'collectibles', 'marketplaces'],
        'metrics': {
          'transactions': 384718,
          'transactionsPercentageChange': 9.74,
          'uaw': 60153,
          'uawPercentageChange': 49.2,
          'volume': 140906.56,
          'volumePercentageChange': -1.98,
          'balance': 60304.02,
          'balancePercentageChange': 16.38
        }
      },
      {
        'dappId': 19943,
        'name': 'IMVU powered by MetaJuice',
        'description':
          'IMVU is the world’s largest web3 social metaverse. Online since 2004, with 350+ million registered accounts and millions of monthly users.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/19943/imvupoweredbymetajuice-dapp-games-19943-logo_f6fdfc9e33a777ef3a951e8a8861f159.png',
        'link': 'https://dappradar.com/dapp/imvu-powered-by-metajuice',
        'website': 'https://metajuice.com/',
        'chains': ['ethereum', 'immutablex', 'polygon'],
        'categories': ['games', 'collectibles', 'marketplaces'],
        'metrics': {
          'transactions': 90240,
          'transactionsPercentageChange': -0.88,
          'uaw': 47894,
          'uawPercentageChange': -2.47,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 17930,
        'name': 'OKX NFT Marketplace',
        'description':
          'OKX NFT marketplace is a decentralized NFT exchange to create, collect and trade extraordinary NFTs',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/17930/okxnftmarketplace-dapp-marketplaces-ethereum-logo_957bf19c8ce39777691821451c778293.png',
        'link': 'https://dappradar.com/dapp/okx-nft-marketplace',
        'website': 'https://www.okx.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'klaytn',
          'optimism',
          'polygon',
          'solana',
          'zksync-era'
        ],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 66665,
          'transactionsPercentageChange': 394.18,
          'uaw': 45482,
          'uawPercentageChange': 1302.9,
          'volume': 612546.83,
          'volumePercentageChange': -24.18,
          'balance': 16.07,
          'balancePercentageChange': 14.3
        }
      },
      {
        'dappId': 13,
        'name': 'OpenSea',
        'description':
          'OpenSea is the first and largest peer-to-peer marketplace.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/13/opensea-dapp-marketplaces-ethereum-logo_a3421ac6e32d529db3c8293f4cfa9bcd.png',
        'link': 'https://dappradar.com/dapp/opensea',
        'website': 'https://opensea.io',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'klaytn',
          'optimism',
          'polygon',
          'solana'
        ],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 239565,
          'transactionsPercentageChange': 9.82,
          'uaw': 43505,
          'uawPercentageChange': 3.69,
          'volume': 1028497.27,
          'volumePercentageChange': -16.23,
          'balance': 72.63,
          'balancePercentageChange': 8.87
        }
      },
      {
        'dappId': 1699,
        'name': 'ParaSwap',
        'description': 'ParaSwap is the fastest DEX aggregator for traders',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/1699/paraswapio-dapp-exchanges-ethereum-logo_dd922d8bafe2f8433bfff4b6721dc066.png',
        'link': 'https://dappradar.com/dapp/paraswap',
        'website': 'https://paraswap.io',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 97602,
          'transactionsPercentageChange': -1.47,
          'uaw': 40014,
          'uawPercentageChange': -16.33,
          'volume': 165525873.52,
          'volumePercentageChange': 22.06,
          'balance': 194661.66,
          'balancePercentageChange': 12.72
        }
      },
      {
        'dappId': 18539,
        'name': 'QuestN',
        'description': 'Marketing, Growth and Analysis for Web3',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18539/questn-dapp-other-ethereum-logo_e8000287aed5bea26ceae151f12eef65.png',
        'link': 'https://dappradar.com/dapp/questn',
        'website': 'https://questn.com/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['social', 'other'],
        'metrics': {
          'transactions': 49328,
          'transactionsPercentageChange': 55.41,
          'uaw': 34245,
          'uawPercentageChange': 89.6,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': 13.55,
          'balancePercentageChange': 0.01
        }
      },
      {
        'dappId': 6017,
        'name': 'QuickSwap',
        'description':
          'QuickSwap is a permissionless decentralized exchange (DEX) based on Ethereum, powered by Matic Network’s Layer 2 scalability infrastructure.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/6017/quickswap-dapp-defi-matic-logo_026b11b3c39cffb979ec834646d39f3c.png',
        'link': 'https://dappradar.com/dapp/quickswap',
        'website': 'https://quickswap.exchange/',
        'chains': ['polygon'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 259771,
          'transactionsPercentageChange': 15.76,
          'uaw': 33669,
          'uawPercentageChange': 19.25,
          'volume': 1461244184.07,
          'volumePercentageChange': 166.15,
          'balance': 87802537.51,
          'balancePercentageChange': 4.95
        }
      },
      {
        'dappId': 257,
        'name': '0x Protocol',
        'description':
          '0x is an open protocol that enables the peer-to-peer exchange of assets on the Ethereum blockchain.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/257/0xprotocol-dapp-other-eth-logo_7e869e1e5fbe2b41499661ea503195bd.png',
        'link': 'https://dappradar.com/dapp/0x-protocol',
        'website': 'https://www.0x.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 106155,
          'transactionsPercentageChange': 2.08,
          'uaw': 33099,
          'uawPercentageChange': -7.17,
          'volume': 60860576.36,
          'volumePercentageChange': 62.07,
          'balance': 5116.04,
          'balancePercentageChange': -14.04
        }
      },
      {
        'dappId': 13115,
        'name': 'KyberSwap Classic',
        'description':
          "KyberSwap is DeFi's first dynamic market maker, providing the best token rates for traders and maximizing returns for LPs, in one decentralized platform",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/13115/kyberswap-dapp-exchanges-ethereum-logo_62de85cf9d622aea207974f11a7134d3.png',
        'link': 'https://dappradar.com/dapp/kyberswap-classic',
        'website': 'https://kyberswap.com/#/about',
        'chains': [
          'avalanche',
          'binance-smart-chain',
          'cronos',
          'ethereum',
          'fantom',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 63101,
          'transactionsPercentageChange': 78.58,
          'uaw': 26941,
          'uawPercentageChange': 80.2,
          'volume': 4644833.53,
          'volumePercentageChange': 32.36,
          'balance': 89709473.33,
          'balancePercentageChange': 12.46
        }
      }
    ]
  },
  'binance_smart_chain': {
    'success': true,
    'chain': 'binance-smart-chain',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 31904,
        'name': 'TinyTap',
        'description':
          "TinyTap, world's largest library of games created by teachers and played by millions of families, now building an open education system on blockchain.",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/31904/tinytap-dapp-other-matic-logo_92b42812ae613d90adcecdb73f524518.png',
        'link': 'https://dappradar.com/dapp/tinytap',
        'website': 'https://www.tinytap.com',
        'chains': ['binance-smart-chain', 'polygon'],
        'categories': ['other'],
        'metrics': {
          'transactions': 1427540,
          'transactionsPercentageChange': 28.73,
          'uaw': 969764,
          'uawPercentageChange': 27.82,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 33108,
        'name': 'PancakeSwap V3',
        'description': 'Trade. Earn. Win. NFT.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/33108/pancakeswapv3-dapp-exchanges-bsc-logo_640c1cad8a042a080d14075f84a6e125.png',
        'link': 'https://dappradar.com/dapp/pancakeswap-v3',
        'website': 'https://pancakeswap.finance/',
        'chains': ['base', 'binance-smart-chain', 'ethereum', 'zksync-era'],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 3099439,
          'transactionsPercentageChange': 5.59,
          'uaw': 563934,
          'uawPercentageChange': 3.62,
          'volume': 5819259600.5,
          'volumePercentageChange': 19.01,
          'balance': 219121209.93,
          'balancePercentageChange': -5.44
        }
      },
      {
        'dappId': 4600,
        'name': 'PancakeSwap V2',
        'description': 'The #1 AMM and yield farm on Binance Smart Chain',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/4600/pancakeswap-dapp-defi-bsc-logo_97d09bb01774bc41263cb848bedbe994.png',
        'link': 'https://dappradar.com/dapp/pancakeswap-v2',
        'website': 'https://pancakeswap.finance/',
        'chains': ['base', 'binance-smart-chain', 'ethereum', 'zksync-era'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 3878337,
          'transactionsPercentageChange': 13.48,
          'uaw': 500895,
          'uawPercentageChange': 16.13,
          'volume': 1868064335.07,
          'volumePercentageChange': 1.86,
          'balance': 74317573.73,
          'balancePercentageChange': 23.74
        }
      },
      {
        'dappId': 18539,
        'name': 'QuestN',
        'description': 'Marketing, Growth and Analysis for Web3',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18539/questn-dapp-other-ethereum-logo_e8000287aed5bea26ceae151f12eef65.png',
        'link': 'https://dappradar.com/dapp/questn',
        'website': 'https://questn.com/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['social', 'other'],
        'metrics': {
          'transactions': 280966,
          'transactionsPercentageChange': 1809,
          'uaw': 210890,
          'uawPercentageChange': 1788.34,
          'volume': 5,
          'volumePercentageChange': 100,
          'balance': 7.01,
          'balancePercentageChange': 250.35
        }
      },
      {
        'dappId': 29295,
        'name': 'Chainspot.io',
        'description':
          'Chainspot router is a smart web3 liquidity aggregator supporting 20 chains, 14 bridges and 3 routers.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/29295/chainspotio-dapp-exchanges-29295-logo_d7c6f77baf5c36acda858f32f1f2948b.png',
        'link': 'https://dappradar.com/dapp/chainspot-io',
        'website': 'https://app.chainspot.io/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 194906,
          'transactionsPercentageChange': 17.85,
          'uaw': 194876,
          'uawPercentageChange': 17.92,
          'volume': 151.45,
          'volumePercentageChange': 1.6,
          'balance': 305.81,
          'balancePercentageChange': 122.68
        }
      },
      {
        'dappId': 20005,
        'name': 'Hooked',
        'description':
          'The on-ramp layer for massive Web3 adoption to form the ecosystem of future community-owned economies.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20005/hooked-dapp-other-bsc-logo_58634484a92c6495019430e287b422c8.png',
        'link': 'https://dappradar.com/dapp/hooked',
        'website': 'https://hooked.io',
        'chains': ['binance-smart-chain'],
        'categories': ['social'],
        'metrics': {
          'transactions': 2141895,
          'transactionsPercentageChange': -13.97,
          'uaw': 189238,
          'uawPercentageChange': -7.02,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': 399.17,
          'balancePercentageChange': 0.05
        }
      },
      {
        'dappId': 18305,
        'name': 'Stargate',
        'description': 'A Composable Omnichain Native Asset Bridge',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png',
        'link': 'https://dappradar.com/dapp/stargate',
        'website': 'https://stargate.finance/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 186066,
          'transactionsPercentageChange': -36.91,
          'uaw': 107939,
          'uawPercentageChange': -33.59,
          'volume': 108938116.51,
          'volumePercentageChange': -28.1,
          'balance': 26928086.82,
          'balancePercentageChange': -43.67
        }
      },
      {
        'dappId': 46421,
        'name': 'Seg Finance',
        'description':
          'Seg Finance is a Blockchain Game in Binance Smart Chain network. Play to Earn Passive BNB Income',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/46421/segfinance-dapp-games-46421-logo_c774266e0c6aec8f0c496006c409dcd1.png',
        'link': 'https://dappradar.com/dapp/seg-finance',
        'website': 'https://segfinance.io',
        'chains': ['binance-smart-chain'],
        'categories': ['games'],
        'metrics': {
          'transactions': 3133637,
          'transactionsPercentageChange': 28.94,
          'uaw': 102469,
          'uawPercentageChange': 30.81,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': 0.21,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 9001,
        'name': 'SecondLive',
        'description': 'SecondLive: Choose Life, Choose Space, Choose Friends.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/9001/secondlive-dapp-games-9001-logo_39b0104f6151b9424ee1f25a5ca23131.png',
        'link': 'https://dappradar.com/dapp/secondlive',
        'website': 'https://secondlive.world/',
        'chains': ['arbitrum', 'binance-smart-chain', 'zksync-era'],
        'categories': ['games'],
        'metrics': {
          'transactions': 1513035,
          'transactionsPercentageChange': 54.41,
          'uaw': 91413,
          'uawPercentageChange': 3.98,
          'volume': 205480.45,
          'volumePercentageChange': 41.07,
          'balance': 3623.1,
          'balancePercentageChange': 10.07
        }
      },
      {
        'dappId': 20598,
        'name': 'Galxe',
        'description': 'The largest Web3 credential data network in the world.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20598/galxe-dapp-social-matic-logo_033b8027624b1a20f188c1e991201392.png',
        'link': 'https://dappradar.com/dapp/galxe',
        'website': 'https://galxe.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'iotex',
          'moonbeam',
          'optimism',
          'polygon'
        ],
        'categories': ['social'],
        'metrics': {
          'transactions': 258302,
          'transactionsPercentageChange': -27.67,
          'uaw': 79767,
          'uawPercentageChange': -39.44,
          'volume': 36553.53,
          'volumePercentageChange': -28.39,
          'balance': 366.56,
          'balancePercentageChange': 14.38
        }
      },
      {
        'dappId': 38075,
        'name': 'Gaimin',
        'description':
          'GAIMIN is a Web3 gaming infrastructure project strategically positioned at the disruptive intersection of Web3, gaming, and cloud computing.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/38075/gaimin-dapp-games-38075-logo_b17dbda02af5d894e491f0e02b46e3f9.png',
        'link': 'https://dappradar.com/dapp/gaimin',
        'website': 'https://gaimin.gg',
        'chains': ['binance-smart-chain'],
        'categories': ['games', 'collectibles', 'marketplaces'],
        'metrics': {
          'transactions': 133815,
          'transactionsPercentageChange': 7.11,
          'uaw': 62273,
          'uawPercentageChange': -0.84,
          'volume': 4669.91,
          'volumePercentageChange': -19.55,
          'balance': 732.95,
          'balancePercentageChange': 1465800
        }
      },
      {
        'dappId': 38982,
        'name': 'QnA3',
        'description':
          'The first crypto data AI platform with a community-centric approach and AI-powered solutions. Get real-time answers from my AI  agent and the community.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/38982/qa3-dapp-other-38982-logo_d8f00aa1925fad4375159a5aebc7911a.png',
        'link': 'https://dappradar.com/dapp/qna3',
        'website': 'https://qna3.ai/',
        'chains': ['binance-smart-chain'],
        'categories': ['other'],
        'metrics': {
          'transactions': 391335,
          'transactionsPercentageChange': 8.42,
          'uaw': 55069,
          'uawPercentageChange': 175.5,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 12510,
        'name': 'PinkSale',
        'description':
          'PinkSale is the #1 launchpad platform raised over $1 billion for 15,000 projects. Total values locked is $300+ million with over 1.7 million of investors.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/12510/pinksale-dapp-defi-bsc-logo_0f4d2cb2d9a8b79d7f78572d68e36143.png',
        'link': 'https://dappradar.com/dapp/pinksale',
        'website': 'https://www.pinksale.finance/',
        'chains': ['binance-smart-chain'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 132602,
          'transactionsPercentageChange': 1.02,
          'uaw': 41845,
          'uawPercentageChange': 3.98,
          'volume': 13562169.43,
          'volumePercentageChange': 93.52,
          'balance': 30743704.52,
          'balancePercentageChange': -31.32
        }
      },
      {
        'dappId': 17343,
        'name': 'CARV',
        'description':
          'CARV is building a credential and data infrastructure focused on gaming, enabling gamers with data sovereignty and games with intelligence',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/17343/carv-dapp-games-matic-logo_be32b3ed491a13d9e9925691a910d6f3.png',
        'link': 'https://dappradar.com/dapp/carv',
        'website': 'https://carv.io/',
        'chains': [
          'avalanche',
          'binance-smart-chain',
          'fantom',
          'other',
          'polygon',
          'zksync-era'
        ],
        'categories': ['social'],
        'metrics': {
          'transactions': 66077,
          'transactionsPercentageChange': 100,
          'uaw': 39722,
          'uawPercentageChange': 100,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 38210,
        'name': 'NFPrompt',
        'description': 'NFPrompt is the first AI prompt UGC platform in Web3!',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/38210/nfprompt-dapp-marketplaces-38210-logo_c684102e2c8d128b6bd35d384f69417a.png',
        'link': 'https://dappradar.com/dapp/nfprompt',
        'website': 'https://nfprompt.io',
        'chains': ['binance-smart-chain'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 606116,
          'transactionsPercentageChange': 9.86,
          'uaw': 34980,
          'uawPercentageChange': 14.73,
          'volume': 60175.61,
          'volumePercentageChange': -37.61,
          'balance': null,
          'balancePercentageChange': null
        }
      }
    ]
  },
  'optimism': {
    'success': true,
    'chain': 'optimism',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 18305,
        'name': 'Stargate',
        'description': 'A Composable Omnichain Native Asset Bridge',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png',
        'link': 'https://dappradar.com/dapp/stargate',
        'website': 'https://stargate.finance/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 279646,
          'transactionsPercentageChange': -41.91,
          'uaw': 169119,
          'uawPercentageChange': -33.78,
          'volume': 131293317.18,
          'volumePercentageChange': -36.06,
          'balance': 21265993.97,
          'balancePercentageChange': 34.79
        }
      },
      {
        'dappId': 7000,
        'name': 'Uniswap V3',
        'description':
          'A protocol for trading and automated liquidity provision on Ethereum.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7000/uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png',
        'link': 'https://dappradar.com/dapp/uniswap-v3',
        'website': 'https://app.uniswap.org/#/swap',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 79768,
          'transactionsPercentageChange': 30.51,
          'uaw': 143867,
          'uawPercentageChange': -7.32,
          'volume': 930760593.48,
          'volumePercentageChange': 4.58,
          'balance': 5005932.02,
          'balancePercentageChange': -7.11
        }
      },
      {
        'dappId': 20598,
        'name': 'Galxe',
        'description': 'The largest Web3 credential data network in the world.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20598/galxe-dapp-social-matic-logo_033b8027624b1a20f188c1e991201392.png',
        'link': 'https://dappradar.com/dapp/galxe',
        'website': 'https://galxe.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'iotex',
          'moonbeam',
          'optimism',
          'polygon'
        ],
        'categories': ['social'],
        'metrics': {
          'transactions': 97167,
          'transactionsPercentageChange': -9.93,
          'uaw': 49717,
          'uawPercentageChange': -3.73,
          'volume': 1.93,
          'volumePercentageChange': 100,
          'balance': 39.16,
          'balancePercentageChange': 4.06
        }
      },
      {
        'dappId': 257,
        'name': '0x Protocol',
        'description':
          '0x is an open protocol that enables the peer-to-peer exchange of assets on the Ethereum blockchain.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/257/0xprotocol-dapp-other-eth-logo_7e869e1e5fbe2b41499661ea503195bd.png',
        'link': 'https://dappradar.com/dapp/0x-protocol',
        'website': 'https://www.0x.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 48461,
          'transactionsPercentageChange': -26.28,
          'uaw': 25008,
          'uawPercentageChange': -31.95,
          'volume': 26252251.99,
          'volumePercentageChange': -16.25,
          'balance': 1232.26,
          'balancePercentageChange': 3.84
        }
      },
      {
        'dappId': 18375,
        'name': 'rhino.fi',
        'description':
          'rhino.fi - The ultimate Layer 2 bridge for low cost & rapid interoperability cross-chain. Bridge. Swap. Invest',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18375/rhinofi-dapp-defi-18375-logo_b9f4cc70975896bd809fd2b738b4c2c2.png',
        'link': 'https://dappradar.com/dapp/rhino-fi',
        'website': 'https://app.rhino.fi',
        'chains': [
          'arbitrum',
          'base',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 26408,
          'transactionsPercentageChange': 880.98,
          'uaw': 23237,
          'uawPercentageChange': 1535.26,
          'volume': 2035307.2,
          'volumePercentageChange': -21.63,
          'balance': 450230.75,
          'balancePercentageChange': -26.41
        }
      },
      {
        'dappId': 43969,
        'name': 'Move Stake',
        'description':
          'Move Stake is a Big Multi Chain Decentralized Staking Yield\n1% - 4% Basic Daily ROI\n23% Referral Rewards in 4 Levels\nfree MVB token and many more',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/43969/movestake-dapp-high-risk-43969-logo_3d820705633cc75d8fe0f187aabc4dfa.png',
        'link': 'https://dappradar.com/dapp/move-stake',
        'website': 'https://movestake.io/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['high-risk'],
        'metrics': {
          'transactions': 49607,
          'transactionsPercentageChange': -29.45,
          'uaw': 22593,
          'uawPercentageChange': -30.83,
          'volume': 288291.51,
          'volumePercentageChange': -37.53,
          'balance': 53359.46,
          'balancePercentageChange': 12.49
        }
      },
      {
        'dappId': 25449,
        'name': 'WOOFi',
        'description':
          'One DEX to rule all chains ⛓️ \n\n👑 Trade and earn like royalty with unmatched execution, cross-chain swaps, and single-sided yields.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/25449/woofi-dapp-exchanges-25449-logo_190c84017f92c8d58cc8897781682504.png',
        'link': 'https://dappradar.com/dapp/woofi',
        'website': 'https://fi.woo.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 72361,
          'transactionsPercentageChange': -8.94,
          'uaw': 18728,
          'uawPercentageChange': -26.25,
          'volume': 74530441.53,
          'volumePercentageChange': 18.1,
          'balance': 825391.82,
          'balancePercentageChange': 5.45
        }
      },
      {
        'dappId': 24665,
        'name': 'Odos',
        'description':
          'Odos is a DEFI aggregator that traverses a large universe of possible token swap combinations and non-linear paths, delivering greater savings to users',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/24665/odosxyz-dapp-defi-ethereum-logo_bca2ab8b544c29ffdea509bd7a3fc20a.png',
        'link': 'https://dappradar.com/dapp/odos',
        'website': 'http://app.odos.xyz',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 24905,
          'transactionsPercentageChange': 17.9,
          'uaw': 13320,
          'uawPercentageChange': 40.24,
          'volume': 25743941.69,
          'volumePercentageChange': -3.68,
          'balance': 30.84,
          'balancePercentageChange': -99.27
        }
      },
      {
        'dappId': 22780,
        'name': 'Jumper Exchange',
        'description': "Crypto's Everything Exchange. Powered by LI.FI",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/22780/jumperexchange-dapp-exchanges-22780-logo_ecabf0c81bb0271db12615535369791f.png',
        'link': 'https://dappradar.com/dapp/jumper-exchange',
        'website': 'https://jumper.exchange/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'cronos',
          'ethereum',
          'fantom',
          'moonbeam',
          'moonriver',
          'optimism',
          'other',
          'polygon'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 20487,
          'transactionsPercentageChange': -19.91,
          'uaw': 12822,
          'uawPercentageChange': -17.62,
          'volume': 22744415.83,
          'volumePercentageChange': -4.16,
          'balance': 0.03,
          'balancePercentageChange': 0.28
        }
      },
      {
        'dappId': 1699,
        'name': 'ParaSwap',
        'description': 'ParaSwap is the fastest DEX aggregator for traders',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/1699/paraswapio-dapp-exchanges-ethereum-logo_dd922d8bafe2f8433bfff4b6721dc066.png',
        'link': 'https://dappradar.com/dapp/paraswap',
        'website': 'https://paraswap.io',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 18542,
          'transactionsPercentageChange': -61.39,
          'uaw': 12793,
          'uawPercentageChange': -68.25,
          'volume': 48571633.66,
          'volumePercentageChange': 51.01,
          'balance': 0,
          'balancePercentageChange': 0
        }
      },
      {
        'dappId': 25959,
        'name': 'Manifold',
        'description': 'Enabling creative sovereignty in web3',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/25959/manifold-dapp-marketplaces-ethereum-logo_47712d1e78deb9d462fa4f012cb750f3.png',
        'link': 'https://dappradar.com/dapp/manifold',
        'website': 'https://studio.manifold.xyz/',
        'chains': ['ethereum', 'optimism'],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 12591,
          'transactionsPercentageChange': 23.57,
          'uaw': 11816,
          'uawPercentageChange': 23.73,
          'volume': 30325.86,
          'volumePercentageChange': 51.07,
          'balance': 228574.03,
          'balancePercentageChange': 14.9
        }
      },
      {
        'dappId': 26710,
        'name': 'Aave V3',
        'description':
          'Welcome to Aave Protocol, an open source and non-custodial liquidity protocol.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/26710/aavev3-dapp-defi-ethereum-logo_41b4a5fc524689812fe4240843c182d3.png',
        'link': 'https://dappradar.com/dapp/aave-v3',
        'website': 'https://app.aave.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 29692,
          'transactionsPercentageChange': -19.47,
          'uaw': 9241,
          'uawPercentageChange': -19.02,
          'volume': 37784260.18,
          'volumePercentageChange': 16.26,
          'balance': 450.74,
          'balancePercentageChange': 100
        }
      },
      {
        'dappId': 14912,
        'name': 'Perpetual Protocol',
        'description':
          'Perpetual Protocol is an on-chain perpetual futures DEX with deep liquidity and builder-ready composability.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/14912/perpetualprotocol-dapp-defi-optimism-logo_71c46a918b7f06a68e2b5d298949f36b.png',
        'link': 'https://dappradar.com/dapp/perpetual-protocol',
        'website': 'https://perp.com',
        'chains': ['ethereum', 'optimism'],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 126965,
          'transactionsPercentageChange': -45.38,
          'uaw': 6005,
          'uawPercentageChange': -55.79,
          'volume': 24305757.03,
          'volumePercentageChange': 22.57,
          'balance': 11526543.92,
          'balancePercentageChange': -10.63
        }
      },
      {
        'dappId': 44959,
        'name': 'Layerswap',
        'description':
          'Fast and reliable crypto transfers across networks and exchanges.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/44959/layerswap-dapp-defi-44959-logo_b89e707bd7b84f4852158beb246ec33c.png',
        'link': 'https://dappradar.com/dapp/layerswap',
        'website': 'https://www.layerswap.io/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'immutablex',
          'optimism',
          'other',
          'polygon',
          'solana',
          'zksync-era'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 5774,
          'transactionsPercentageChange': 2.61,
          'uaw': 5656,
          'uawPercentageChange': 6.08,
          'volume': 1917997.71,
          'volumePercentageChange': -80.41,
          'balance': 178480.03,
          'balancePercentageChange': -49.57
        }
      },
      {
        'dappId': 18466,
        'name': 'Velodrome',
        'description': 'The central trading and liquidity marketplace on',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18466/velodrome-dapp-defi-optimism-logo_66e079424a31aab935fd838e9845f084.png',
        'link': 'https://dappradar.com/dapp/velodrome',
        'website': 'https://app.velodrome.finance/',
        'chains': ['optimism'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 36922,
          'transactionsPercentageChange': -3.27,
          'uaw': 5626,
          'uawPercentageChange': -12.01,
          'volume': 2456211.53,
          'volumePercentageChange': -0.64,
          'balance': null,
          'balancePercentageChange': null
        }
      }
    ]
  },
  'aurora': {
    'success': true,
    'chain': 'aurora',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 43716,
        'name': 'PipeFlare',
        'description':
          'PipeFlare is the leading Web3 casual gaming site hosting 17+ play-to earn games and thousands of dollars in weekly leaderboard prizes.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/43716/pipeflare-dapp-games-43716-logo_030ad5a8470ceea7b690da61147fd855.png',
        'link': 'https://dappradar.com/dapp/pipeflare',
        'website': 'https://www.pipeflare.io',
        'chains': ['aurora', 'avalanche', 'polygon'],
        'categories': ['games', 'collectibles'],
        'metrics': {
          'transactions': 320452,
          'transactionsPercentageChange': 24.96,
          'uaw': 279447,
          'uawPercentageChange': 21.48,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 30927,
        'name': 'motoDEX',
        'description':
          'MotoDEX is a Blockchain Game, in which users participate in motorcycle races, \ndevelop their riders and improve high-speed tracks.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/30927/motodex-dapp-games-aurora-logo_9fc42a38e10cde705d67a9b7bf4982f1.png',
        'link': 'https://dappradar.com/dapp/motodex',
        'website': 'https://motodex.openbisea.com?chain=aurora',
        'chains': ['aurora', 'base', 'eosevm', 'near', 'polygon', 'skale'],
        'categories': ['games'],
        'metrics': {
          'transactions': 16765,
          'transactionsPercentageChange': 10.47,
          'uaw': 16760,
          'uawPercentageChange': 10.49,
          'volume': 46057.42,
          'volumePercentageChange': 13.92,
          'balance': 214.91,
          'balancePercentageChange': 10.02
        }
      },
      {
        'dappId': 15969,
        'name': 'Aurigami',
        'description': 'Lend, borrow, and earn with ease on Aurora.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/15969/aurigami-dapp-defi-aurora-logo_9b4f52ea5571d2450de7cebfb31b61c5.png',
        'link': 'https://dappradar.com/dapp/aurigami',
        'website': 'https://www.aurigami.finance/',
        'chains': ['aurora'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 12898,
          'transactionsPercentageChange': -88.02,
          'uaw': 593,
          'uawPercentageChange': 165.92,
          'volume': 10400807.27,
          'volumePercentageChange': 21.99,
          'balance': 1288085.93,
          'balancePercentageChange': -1.9
        }
      },
      {
        'dappId': 1528,
        'name': '1inch Network',
        'description':
          'A distributed DeFi aggregator for various protocols on multiple chains.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/1528/1inchexchange-dapp-exchanges-ethereum-logo_855963bbf18d4c089e92160b47b43342.png',
        'link': 'https://dappradar.com/dapp/1inch-network',
        'website': 'https://app.1inch.io',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'klaytn',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 1666,
          'transactionsPercentageChange': -67.61,
          'uaw': 479,
          'uawPercentageChange': -85.35,
          'volume': 117067.29,
          'volumePercentageChange': 34.08,
          'balance': 0,
          'balancePercentageChange': 0
        }
      },
      {
        'dappId': 20797,
        'name': 'Aurora plus',
        'description': 'Aurora+ is a membership program for users of Aurora',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20797/auroraplus-dapp-defi-aurora-logo_772f084c73962937aa4dd8bd9537b4e8.png',
        'link': 'https://dappradar.com/dapp/aurora-plus',
        'website': 'https://aurora.plus/',
        'chains': ['aurora'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 672,
          'transactionsPercentageChange': 38.56,
          'uaw': 429,
          'uawPercentageChange': -46.11,
          'volume': 293562.89,
          'volumePercentageChange': 119.52,
          'balance': 6750712.8,
          'balancePercentageChange': 66.15
        }
      },
      {
        'dappId': 15961,
        'name': 'Trisolaris',
        'description': "#1 Dex on NEAR's Aurora EVM",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/15961/trisolaris-dapp-defi-aurora-logo_fb4ce6f4cdc518b36d563c777ad35361.png',
        'link': 'https://dappradar.com/dapp/trisolaris',
        'website': 'https://www.trisolaris.io/#/swap',
        'chains': ['aurora'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 1507,
          'transactionsPercentageChange': -13.09,
          'uaw': 345,
          'uawPercentageChange': 0.58,
          'volume': 11218.8,
          'volumePercentageChange': 3.1,
          'balance': 53859.55,
          'balancePercentageChange': -13.81
        }
      },
      {
        'dappId': 6253,
        'name': 'tofuNFT',
        'description':
          'NFT marketplace focused on GameFi & collectibles on multi-chain. Formerly, SCV NFT Market.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/6253/tofunft-dapp-marketplaces-bsc-logo_7690ef67e4db87e445c4dd8552595716.png',
        'link': 'https://dappradar.com/dapp/tofunft',
        'website': 'https://tofunft.com/',
        'chains': [
          'arbitrum',
          'astar',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'cronos',
          'ethereum',
          'fantom',
          'klaytn',
          'moonbeam',
          'moonriver',
          'near',
          'oasis',
          'optimism',
          'polygon',
          'shiden',
          'telosevm'
        ],
        'categories': ['marketplaces'],
        'metrics': {
          'transactions': 594,
          'transactionsPercentageChange': -1.33,
          'uaw': 147,
          'uawPercentageChange': 81.48,
          'volume': 8859.05,
          'volumePercentageChange': 32.49,
          'balance': 697.75,
          'balancePercentageChange': -23.05
        }
      },
      {
        'dappId': 39205,
        'name': 'HuntersBet',
        'description':
          'Hunters Bet is a decentralized platform designed for event forecasting and tournament creation based on predictions.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/39205/huntersbet-dapp-games-39205-logo_e12719cd40a75e8ed9a90f76f77808e5.png',
        'link': 'https://dappradar.com/dapp/huntersbet',
        'website': 'https://auroraplay.app/tournaments',
        'chains': ['aurora'],
        'categories': ['gambling'],
        'metrics': {
          'transactions': 1189,
          'transactionsPercentageChange': 0.51,
          'uaw': 140,
          'uawPercentageChange': -2.78,
          'volume': 42.78,
          'volumePercentageChange': -34.65,
          'balance': 109.1,
          'balancePercentageChange': 85.23
        }
      },
      {
        'dappId': 34360,
        'name': 'Aurora 2048',
        'description':
          '2048 is a popular puzzle game that has been adapted for play on the Aurora blockchain.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/34360/aurora2048-dapp-games-aurora-logo_d0896319782b41ea137a9614a98c3806.png',
        'link': 'https://dappradar.com/dapp/aurora-2048',
        'website': 'https://twenty48.app/',
        'chains': ['aurora'],
        'categories': ['games'],
        'metrics': {
          'transactions': 1589,
          'transactionsPercentageChange': 30.67,
          'uaw': 121,
          'uawPercentageChange': -33.15,
          'volume': 0,
          'volumePercentageChange': -100,
          'balance': 1.45,
          'balancePercentageChange': 31.82
        }
      },
      {
        'dappId': 12023,
        'name': 'WannaSwap',
        'description':
          "WannaSwap is the Liquidity Central on NEAR's Aurora EVM",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/12023/wannaswap-dapp-exchanges-other-logo_9964ba5e048b866614fb7875477bad71.png',
        'link': 'https://dappradar.com/dapp/wannaswap',
        'website': 'https://wannaswap.finance',
        'chains': ['aurora'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 823,
          'transactionsPercentageChange': 82.08,
          'uaw': 108,
          'uawPercentageChange': 20,
          'volume': 2957.23,
          'volumePercentageChange': 97.99,
          'balance': 15713.91,
          'balancePercentageChange': 29.27
        }
      },
      {
        'dappId': 18539,
        'name': 'QuestN',
        'description': 'Marketing, Growth and Analysis for Web3',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18539/questn-dapp-other-ethereum-logo_e8000287aed5bea26ceae151f12eef65.png',
        'link': 'https://dappradar.com/dapp/questn',
        'website': 'https://questn.com/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['social', 'other'],
        'metrics': {
          'transactions': 96,
          'transactionsPercentageChange': -74.47,
          'uaw': 96,
          'uawPercentageChange': -73.55,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 29204,
        'name': 'WordleAurora',
        'description': 'wordle game on Aurora chain',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/29204/wordleaurora-dapp-games-aurora-logo_82e94ef9998e77ef6df6571a694c7d53.png',
        'link': 'https://dappradar.com/dapp/wordleaurora',
        'website': 'https://wordleaurora.com/',
        'chains': ['aurora'],
        'categories': ['games'],
        'metrics': {
          'transactions': 919,
          'transactionsPercentageChange': 14.16,
          'uaw': 95,
          'uawPercentageChange': -18.8,
          'volume': 0,
          'volumePercentageChange': 0,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 15972,
        'name': 'Bastion',
        'description':
          'The Liquidity Foundation of Aurora. \nLending 🗿 Stableswap.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/15972/bastion-dapp-defi-aurora-logo_be8f6800eefaf19b66aec7d56c49f201.png',
        'link': 'https://dappradar.com/dapp/bastion',
        'website': 'https://bastionprotocol.com/',
        'chains': ['aurora'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 152,
          'transactionsPercentageChange': -11.11,
          'uaw': 75,
          'uawPercentageChange': -13.79,
          'volume': 0,
          'volumePercentageChange': -100,
          'balance': 3893202.82,
          'balancePercentageChange': 2.45
        }
      },
      {
        'dappId': 39542,
        'name': 'Meson.fi',
        'description':
          'Meson provides minute-fast swaps with almost-zero fee & slippage across all major blockchains.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/39542/mesonfinance-dapp-exchanges-39542-logo_0e4e19ba80cf7055633917cc8dd36d92.png',
        'link': 'https://dappradar.com/dapp/meson-fi',
        'website': 'https://meson.fi/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'base',
          'binance-smart-chain',
          'cronos',
          'eosevm',
          'ethereum',
          'fantom',
          'moonbeam',
          'moonriver',
          'optimism',
          'polygon',
          'skale',
          'tron',
          'zksync-era'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 341,
          'transactionsPercentageChange': -55.42,
          'uaw': 75,
          'uawPercentageChange': -53.99,
          'volume': 113973.94,
          'volumePercentageChange': -63.03,
          'balance': 27622.39,
          'balancePercentageChange': -33.89
        }
      },
      {
        'dappId': 23061,
        'name': 'Polaris DEX',
        'description':
          "Be a part of a decentralised exchange using Balancer's AMM. Experience efficient trading, collect fees, earn yield  and much more.",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/23061/polarisdex-dapp-defi-aurora-logo_6ec896bc7d53453739781de969f6fc21.png',
        'link': 'https://dappradar.com/dapp/polaris-dex',
        'website': 'https://polarisfinance.io/',
        'chains': ['aurora'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 236,
          'transactionsPercentageChange': -35.87,
          'uaw': 59,
          'uawPercentageChange': 1.72,
          'volume': 12294.41,
          'volumePercentageChange': -0.56,
          'balance': 226149.01,
          'balancePercentageChange': 13.66
        }
      }
    ]
  },
  'avalanche': {
    'success': true,
    'chain': 'avalanche',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 18305,
        'name': 'Stargate',
        'description': 'A Composable Omnichain Native Asset Bridge',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png',
        'link': 'https://dappradar.com/dapp/stargate',
        'website': 'https://stargate.finance/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 189238,
          'transactionsPercentageChange': -31.73,
          'uaw': 113336,
          'uawPercentageChange': -29.69,
          'volume': 168802575.92,
          'volumePercentageChange': -31.36,
          'balance': 27691128.04,
          'balancePercentageChange': 26.09
        }
      },
      {
        'dappId': 9134,
        'name': 'Trader Joe',
        'description':
          'Trader Joe is powered by the Liquidity Book AMM, the most capital efficient AMM in DeFi. Swap, yield farm, lending, shop for NFTs.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/9134/traderjoe-dapp-exchanges-avalanche-logo_b38e25f54a3c8c4cc65b4dea67272464.png',
        'link': 'https://dappradar.com/dapp/trader-joe',
        'website': 'https://www.traderjoexyz.com/#/home',
        'chains': ['arbitrum', 'avalanche', 'binance-smart-chain', 'ethereum'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 191570,
          'transactionsPercentageChange': -5.2,
          'uaw': 51861,
          'uawPercentageChange': -27.03,
          'volume': 86589614.9,
          'volumePercentageChange': 136.2,
          'balance': 39947782.13,
          'balancePercentageChange': 18.59
        }
      },
      {
        'dappId': 43621,
        'name': 'Stars Arena',
        'description': 'Join the Arena and become a Star.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/43621/starshares-dapp-social-43621-logo_aed7438b599d243e5d56a0e9c10b29bf.png',
        'link': 'https://dappradar.com/dapp/stars-arena',
        'website': 'https://www.starsarena.com',
        'chains': ['avalanche'],
        'categories': ['social'],
        'metrics': {
          'transactions': 1300251,
          'transactionsPercentageChange': 238.43,
          'uaw': 32988,
          'uawPercentageChange': 297.45,
          'volume': 15447878.74,
          'volumePercentageChange': 690.62,
          'balance': 460012.23,
          'balancePercentageChange': -28.46
        }
      },
      {
        'dappId': 20598,
        'name': 'Galxe',
        'description': 'The largest Web3 credential data network in the world.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20598/galxe-dapp-social-matic-logo_033b8027624b1a20f188c1e991201392.png',
        'link': 'https://dappradar.com/dapp/galxe',
        'website': 'https://galxe.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'iotex',
          'moonbeam',
          'optimism',
          'polygon'
        ],
        'categories': ['social'],
        'metrics': {
          'transactions': 62640,
          'transactionsPercentageChange': 64.42,
          'uaw': 32220,
          'uawPercentageChange': 33.45,
          'volume': 0.06,
          'volumePercentageChange': 21,
          'balance': null,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 25449,
        'name': 'WOOFi',
        'description':
          'One DEX to rule all chains ⛓️ \n\n👑 Trade and earn like royalty with unmatched execution, cross-chain swaps, and single-sided yields.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/25449/woofi-dapp-exchanges-25449-logo_190c84017f92c8d58cc8897781682504.png',
        'link': 'https://dappradar.com/dapp/woofi',
        'website': 'https://fi.woo.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 82297,
          'transactionsPercentageChange': 8.25,
          'uaw': 23683,
          'uawPercentageChange': -18.13,
          'volume': 120040331.58,
          'volumePercentageChange': 121.6,
          'balance': 8469976.03,
          'balancePercentageChange': 10.26
        }
      },
      {
        'dappId': 43969,
        'name': 'Move Stake',
        'description':
          'Move Stake is a Big Multi Chain Decentralized Staking Yield\n1% - 4% Basic Daily ROI\n23% Referral Rewards in 4 Levels\nfree MVB token and many more',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/43969/movestake-dapp-high-risk-43969-logo_3d820705633cc75d8fe0f187aabc4dfa.png',
        'link': 'https://dappradar.com/dapp/move-stake',
        'website': 'https://movestake.io/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['high-risk'],
        'metrics': {
          'transactions': 36877,
          'transactionsPercentageChange': -23.42,
          'uaw': 16229,
          'uawPercentageChange': -25.08,
          'volume': 73264.24,
          'volumePercentageChange': -23.75,
          'balance': 30043.05,
          'balancePercentageChange': 150.74
        }
      },
      {
        'dappId': 257,
        'name': '0x Protocol',
        'description':
          '0x is an open protocol that enables the peer-to-peer exchange of assets on the Ethereum blockchain.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/257/0xprotocol-dapp-other-eth-logo_7e869e1e5fbe2b41499661ea503195bd.png',
        'link': 'https://dappradar.com/dapp/0x-protocol',
        'website': 'https://www.0x.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 15773,
          'transactionsPercentageChange': -2.98,
          'uaw': 10558,
          'uawPercentageChange': -3.12,
          'volume': 20807436.06,
          'volumePercentageChange': 59.89,
          'balance': 45.68,
          'balancePercentageChange': 22.11
        }
      },
      {
        'dappId': 1699,
        'name': 'ParaSwap',
        'description': 'ParaSwap is the fastest DEX aggregator for traders',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/1699/paraswapio-dapp-exchanges-ethereum-logo_dd922d8bafe2f8433bfff4b6721dc066.png',
        'link': 'https://dappradar.com/dapp/paraswap',
        'website': 'https://paraswap.io',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 11566,
          'transactionsPercentageChange': -19.79,
          'uaw': 5828,
          'uawPercentageChange': -35.42,
          'volume': 118734160.96,
          'volumePercentageChange': 36.45,
          'balance': 57285.19,
          'balancePercentageChange': 8.74
        }
      },
      {
        'dappId': 7000,
        'name': 'Uniswap V3',
        'description':
          'A protocol for trading and automated liquidity provision on Ethereum.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7000/uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png',
        'link': 'https://dappradar.com/dapp/uniswap-v3',
        'website': 'https://app.uniswap.org/#/swap',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 0,
          'transactionsPercentageChange': 0,
          'uaw': 5062,
          'uawPercentageChange': -20.36,
          'volume': 51969762.62,
          'volumePercentageChange': 284.77,
          'balance': 354679.2,
          'balancePercentageChange': 61.92
        }
      },
      {
        'dappId': 22780,
        'name': 'Jumper Exchange',
        'description': "Crypto's Everything Exchange. Powered by LI.FI",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/22780/jumperexchange-dapp-exchanges-22780-logo_ecabf0c81bb0271db12615535369791f.png',
        'link': 'https://dappradar.com/dapp/jumper-exchange',
        'website': 'https://jumper.exchange/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'cronos',
          'ethereum',
          'fantom',
          'moonbeam',
          'moonriver',
          'optimism',
          'other',
          'polygon'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 7806,
          'transactionsPercentageChange': -10.22,
          'uaw': 4654,
          'uawPercentageChange': -21.82,
          'volume': 6025819.6,
          'volumePercentageChange': -67.58,
          'balance': 0.08,
          'balancePercentageChange': 14.28
        }
      },
      {
        'dappId': 26710,
        'name': 'Aave V3',
        'description':
          'Welcome to Aave Protocol, an open source and non-custodial liquidity protocol.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/26710/aavev3-dapp-defi-ethereum-logo_41b4a5fc524689812fe4240843c182d3.png',
        'link': 'https://dappradar.com/dapp/aave-v3',
        'website': 'https://app.aave.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 19373,
          'transactionsPercentageChange': 36.64,
          'uaw': 4030,
          'uawPercentageChange': -5.51,
          'volume': 27075742.82,
          'volumePercentageChange': 62.73,
          'balance': 860.49,
          'balancePercentageChange': 0.17
        }
      },
      {
        'dappId': 20356,
        'name': 'GMX',
        'description':
          'Decentralised Perpetual Exchange. Trade BTC, ETH, AVAX and more with up to 30x leverage directly from your wallet',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/20356/gmx-dapp-defi-arbitrum-logo_035cc7abf5542f577ca04d1617bd953b.png',
        'link': 'https://dappradar.com/dapp/gmx',
        'website': 'https://gmx.io/#/',
        'chains': ['arbitrum', 'avalanche'],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 28310,
          'transactionsPercentageChange': 75.75,
          'uaw': 3445,
          'uawPercentageChange': -31.28,
          'volume': 69333962.48,
          'volumePercentageChange': 113.95,
          'balance': 13588305.78,
          'balancePercentageChange': -28.38
        }
      },
      {
        'dappId': 5390,
        'name': 'OpenOcean',
        'description':
          'OpenOcean is a leading DEX Aggregator, a cross-chain swap aggregator in the crypto space, offering best swap rate across 23+ networks and 285+ deep liquidities.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/5390/openocean-dapp-defi-ethereum-logo_321024781bf0a84d5e9bd2940477dc4d.png',
        'link': 'https://dappradar.com/dapp/openocean',
        'website': 'https://openocean.finance/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'cronos',
          'ethereum',
          'fantom',
          'moonriver',
          'ontology',
          'optimism',
          'polygon',
          'solana',
          'tron'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 3416,
          'transactionsPercentageChange': -30.99,
          'uaw': 2733,
          'uawPercentageChange': -29.36,
          'volume': 194171.32,
          'volumePercentageChange': -41.39,
          'balance': 201.77,
          'balancePercentageChange': 19.27
        }
      },
      {
        'dappId': 24665,
        'name': 'Odos',
        'description':
          'Odos is a DEFI aggregator that traverses a large universe of possible token swap combinations and non-linear paths, delivering greater savings to users',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/24665/odosxyz-dapp-defi-ethereum-logo_bca2ab8b544c29ffdea509bd7a3fc20a.png',
        'link': 'https://dappradar.com/dapp/odos',
        'website': 'http://app.odos.xyz',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 5470,
          'transactionsPercentageChange': -13.68,
          'uaw': 2522,
          'uawPercentageChange': -26.62,
          'volume': 7391071.37,
          'volumePercentageChange': 37.77,
          'balance': 15.27,
          'balancePercentageChange': -99.31
        }
      },
      {
        'dappId': 7055,
        'name': 'Pangolin Exchange',
        'description':
          'Pangolin is a community-driven decentralized exchange with fast settlement & low transaction fees.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7055/pangolinexchange-dapp-defi-avalanche-logo_8a58d8528470b05c680dcfe6d5078151.png',
        'link': 'https://dappradar.com/dapp/pangolin-exchange',
        'website': 'https://pangolin.exchange/',
        'chains': ['avalanche', 'hedera'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 22054,
          'transactionsPercentageChange': 73.52,
          'uaw': 2224,
          'uawPercentageChange': 0.36,
          'volume': 5004431.92,
          'volumePercentageChange': 186.52,
          'balance': 554.27,
          'balancePercentageChange': 11.25
        }
      }
    ]
  },
  'fantom': {
    'success': true,
    'chain': 'fantom',
    'category': null,
    'range': '30d',
    'top': 15,
    'results': [
      {
        'dappId': 18305,
        'name': 'Stargate',
        'description': 'A Composable Omnichain Native Asset Bridge',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png',
        'link': 'https://dappradar.com/dapp/stargate',
        'website': 'https://stargate.finance/',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 47589,
          'transactionsPercentageChange': -21.94,
          'uaw': 24156,
          'uawPercentageChange': -34.6,
          'volume': 18235.49,
          'volumePercentageChange': -32.14,
          'balance': 167790.01,
          'balancePercentageChange': -12.05
        }
      },
      {
        'dappId': 7543,
        'name': 'SpookySwap',
        'description':
          'SpookySwap is the leading AMM on Fantom, utilizing the chain’s low fees and fast transaction times to create a seamless swapping experience.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/7543/spookyswap-dapp-exchanges-other-logo_9230c2186765e24633af83cc40abcece.png',
        'link': 'https://dappradar.com/dapp/spookyswap',
        'website': 'https://spooky.fi',
        'chains': ['avalanche', 'ethereum', 'fantom'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 59215,
          'transactionsPercentageChange': 12.44,
          'uaw': 12769,
          'uawPercentageChange': -18.52,
          'volume': 61423325.93,
          'volumePercentageChange': 117.81,
          'balance': 25974190.81,
          'balancePercentageChange': 4.23
        }
      },
      {
        'dappId': 257,
        'name': '0x Protocol',
        'description':
          '0x is an open protocol that enables the peer-to-peer exchange of assets on the Ethereum blockchain.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/257/0xprotocol-dapp-other-eth-logo_7e869e1e5fbe2b41499661ea503195bd.png',
        'link': 'https://dappradar.com/dapp/0x-protocol',
        'website': 'https://www.0x.org/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 10999,
          'transactionsPercentageChange': -63.9,
          'uaw': 8247,
          'uawPercentageChange': 5.43,
          'volume': 1289278.24,
          'volumePercentageChange': 16.87,
          'balance': 487.65,
          'balancePercentageChange': 22.19
        }
      },
      {
        'dappId': 43751,
        'name': 'Estfor Kingdom',
        'description':
          'Estfor Kingdom is a medieval idle blockchain-based browser game. Train skills, battle monsters, join clans and complete quests in this free-to-play MMORPG!',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/43751/estforkingdom-dapp-games-43751-logo_45058af3c4c6691ce7155b6be464450c.png',
        'link': 'https://dappradar.com/dapp/estfor-kingdom',
        'website': 'https://estfor.com/',
        'chains': ['fantom'],
        'categories': ['games', 'collectibles'],
        'metrics': {
          'transactions': 211005,
          'transactionsPercentageChange': -58.22,
          'uaw': 7732,
          'uawPercentageChange': -64.33,
          'volume': 4534.06,
          'volumePercentageChange': -76.89,
          'balance': 741.54,
          'balancePercentageChange': -21.66
        }
      },
      {
        'dappId': 13308,
        'name': 'WigoSwap',
        'description':
          'One-stop-shop for all things DeFi on the Fantom network.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/13308/wigoswap-dapp-defi-fantom-logo_baadd7771a534b25453f6f617681196a.png',
        'link': 'https://dappradar.com/dapp/wigoswap',
        'website': 'https://wigoswap.io',
        'chains': ['fantom'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 15682,
          'transactionsPercentageChange': 133.78,
          'uaw': 3074,
          'uawPercentageChange': 51.73,
          'volume': 2684134.58,
          'volumePercentageChange': 190.14,
          'balance': 1499240.43,
          'balancePercentageChange': 18.03
        }
      },
      {
        'dappId': 46325,
        'name': 'DappGate',
        'description':
          'DappGate is a bridging tool which uses ONFTs and OFTs across 26+ networks, smooth and problem free transactions, with a low cost.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/46325/dappgate-dapp-defi-46325-logo_b6f9319e722b11bc0bb922eb0a139977.png',
        'link': 'https://dappradar.com/dapp/dappgate',
        'website': 'https://dappgate.io/apps/dappgate',
        'chains': [
          'arbitrum',
          'astar',
          'aurora',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'defikingdoms',
          'fantom',
          'klaytn',
          'moonbeam',
          'moonriver',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['defi'],
        'metrics': {
          'transactions': 4877,
          'transactionsPercentageChange': 5.45,
          'uaw': 1696,
          'uawPercentageChange': -33.8,
          'volume': 810.82,
          'volumePercentageChange': -13.25,
          'balance': 0.23,
          'balancePercentageChange': null
        }
      },
      {
        'dappId': 1699,
        'name': 'ParaSwap',
        'description': 'ParaSwap is the fastest DEX aggregator for traders',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/1699/paraswapio-dapp-exchanges-ethereum-logo_dd922d8bafe2f8433bfff4b6721dc066.png',
        'link': 'https://dappradar.com/dapp/paraswap',
        'website': 'https://paraswap.io',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 5553,
          'transactionsPercentageChange': 51.8,
          'uaw': 1689,
          'uawPercentageChange': -2.71,
          'volume': 5687610.65,
          'volumePercentageChange': 142,
          'balance': 7700.68,
          'balancePercentageChange': 14.54
        }
      },
      {
        'dappId': 23679,
        'name': 'Equalizer',
        'description':
          'Equalizer Exchange is a DEX that uses a vote escrowed model to drive Liquidity to the highest volume pairs.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/23679/equalizer-dapp-exchanges-fantom-logo_3f9e9725c173a3b936b52852aa80272d.png',
        'link': 'https://dappradar.com/dapp/equalizer',
        'website': 'https://equalizer.exchange/',
        'chains': ['base', 'fantom'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 4386,
          'transactionsPercentageChange': 9.29,
          'uaw': 1428,
          'uawPercentageChange': -2.06,
          'volume': 1869395.09,
          'volumePercentageChange': 57.57,
          'balance': 640254.04,
          'balancePercentageChange': 0.64
        }
      },
      {
        'dappId': 17293,
        'name': 'Beethoven X',
        'description':
          'Your favorite decentralized exchange on Fantom and Optimism.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/17293/beethovenx-dapp-defi-fantom-logo_897903118bc07affa80a155e81f5d8b3.png',
        'link': 'https://dappradar.com/dapp/beethoven-x',
        'website': 'https://beets.fi/#/',
        'chains': ['fantom'],
        'categories': ['defi'],
        'metrics': {
          'transactions': 2610,
          'transactionsPercentageChange': 19.51,
          'uaw': 1265,
          'uawPercentageChange': 0.88,
          'volume': 15100047.64,
          'volumePercentageChange': 176.36,
          'balance': 10140165.49,
          'balancePercentageChange': 3.91
        }
      },
      {
        'dappId': 22780,
        'name': 'Jumper Exchange',
        'description': "Crypto's Everything Exchange. Powered by LI.FI",
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/22780/jumperexchange-dapp-exchanges-22780-logo_ecabf0c81bb0271db12615535369791f.png',
        'link': 'https://dappradar.com/dapp/jumper-exchange',
        'website': 'https://jumper.exchange/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'base',
          'binance-smart-chain',
          'celo',
          'cronos',
          'ethereum',
          'fantom',
          'moonbeam',
          'moonriver',
          'optimism',
          'other',
          'polygon'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 1767,
          'transactionsPercentageChange': -33.14,
          'uaw': 1171,
          'uawPercentageChange': -35.3,
          'volume': 478844.52,
          'volumePercentageChange': -62.61,
          'balance': 0.07,
          'balancePercentageChange': 16.75
        }
      },
      {
        'dappId': 17863,
        'name': 'PaintSwap',
        'description':
          'PaintSwap is an open decentralised NFT marketplace supporting all NFTs from art to fNFTs multiple standards and more!',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/17863/paintswap-dapp-marketplaces-fantom-logo_090c836389d078cc7c0b23ee3fe6d0af.png',
        'link': 'https://dappradar.com/dapp/paintswap',
        'website': 'https://paintswap.finance/',
        'chains': ['arbitrum', 'fantom', 'polygon'],
        'categories': ['marketplaces', 'defi'],
        'metrics': {
          'transactions': 7138,
          'transactionsPercentageChange': -43.9,
          'uaw': 1130,
          'uawPercentageChange': -16.85,
          'volume': 23736.24,
          'volumePercentageChange': -45.59,
          'balance': 1439.36,
          'balancePercentageChange': 19.14
        }
      },
      {
        'dappId': 5390,
        'name': 'OpenOcean',
        'description':
          'OpenOcean is a leading DEX Aggregator, a cross-chain swap aggregator in the crypto space, offering best swap rate across 23+ networks and 285+ deep liquidities.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/5390/openocean-dapp-defi-ethereum-logo_321024781bf0a84d5e9bd2940477dc4d.png',
        'link': 'https://dappradar.com/dapp/openocean',
        'website': 'https://openocean.finance/',
        'chains': [
          'arbitrum',
          'aurora',
          'avalanche',
          'binance-smart-chain',
          'cronos',
          'ethereum',
          'fantom',
          'moonriver',
          'ontology',
          'optimism',
          'polygon',
          'solana',
          'tron'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 2655,
          'transactionsPercentageChange': -9.54,
          'uaw': 1126,
          'uawPercentageChange': 24.01,
          'volume': 45847.57,
          'volumePercentageChange': -27.26,
          'balance': 10.01,
          'balancePercentageChange': 0.12
        }
      },
      {
        'dappId': 4359,
        'name': 'SushiSwap',
        'description': 'An evolution of Uniswap with SUSHI tokenomics.',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/4359/sushi-dapp-defi-ethereum-logo_be947ddb92b398a20f1dc921dd9699c4.png',
        'link': 'https://dappradar.com/dapp/sushiswap',
        'website': 'https://app.sushi.com/',
        'chains': [
          'arbitrum',
          'avalanche',
          'binance-smart-chain',
          'celo',
          'ethereum',
          'fantom',
          'moonriver',
          'polygon',
          'telosevm'
        ],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 2661,
          'transactionsPercentageChange': 165.04,
          'uaw': 835,
          'uawPercentageChange': 38.94,
          'volume': 92567.25,
          'volumePercentageChange': 82.12,
          'balance': 3990.1,
          'balancePercentageChange': 14.43
        }
      },
      {
        'dappId': 24665,
        'name': 'Odos',
        'description':
          'Odos is a DEFI aggregator that traverses a large universe of possible token swap combinations and non-linear paths, delivering greater savings to users',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/24665/odosxyz-dapp-defi-ethereum-logo_bca2ab8b544c29ffdea509bd7a3fc20a.png',
        'link': 'https://dappradar.com/dapp/odos',
        'website': 'http://app.odos.xyz',
        'chains': [
          'arbitrum',
          'avalanche',
          'base',
          'binance-smart-chain',
          'ethereum',
          'fantom',
          'optimism',
          'polygon',
          'zksync-era'
        ],
        'categories': ['exchanges', 'defi'],
        'metrics': {
          'transactions': 1204,
          'transactionsPercentageChange': -8.09,
          'uaw': 631,
          'uawPercentageChange': -24.97,
          'volume': 630035.77,
          'volumePercentageChange': -11.94,
          'balance': 184.46,
          'balancePercentageChange': 21.66
        }
      },
      {
        'dappId': 16410,
        'name': 'SpiritSwap',
        'description':
          'Most versatile DEX & DeFi suite on Fantom 🚀 \nUnleash Your DeFi Spirit!',
        'logo':
          'https://dashboard-assets.dappradar.com/geRRcWz9IdTwSrm1/document/16410/spiritswap-dapp-defi-fantom-logo_63eadd05d5163d7ec48ac6f1afaf438b.png',
        'link': 'https://dappradar.com/dapp/spiritswap',
        'website': 'https://app.spiritswap.finance/#/',
        'chains': ['fantom'],
        'categories': ['exchanges'],
        'metrics': {
          'transactions': 2883,
          'transactionsPercentageChange': 19.93,
          'uaw': 592,
          'uawPercentageChange': 12.76,
          'volume': 92456.12,
          'volumePercentageChange': -40,
          'balance': 549733.52,
          'balancePercentageChange': 32.52
        }
      }
    ]
  }
}
