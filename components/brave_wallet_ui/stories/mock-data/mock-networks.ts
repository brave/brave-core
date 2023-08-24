// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../constants/types'
import {
  BTCIconUrl,
  ETHIconUrl,
  FILECOINIconUrl,
  SOLIconUrl
} from './asset-icons'

export const mockEthMainnet: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['https://etherscan.io', 'https://etherchain.org'],
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  chainName: 'Ethereum Mainnet',
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
  decimals: 18,
  iconUrls: [ETHIconUrl],
  isEip1559: true,
  rpcEndpoints: [{ url: 'https://mainnet.infura.io/v3/' }],
  symbol: 'ETH',
  symbolName: 'Ethereum'
}

export const mockGoerli: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['https://goerli.etherscan.io'],
  chainId: BraveWallet.GOERLI_CHAIN_ID,
  chainName: 'Goerli Test Network',
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
  decimals: 18,
  iconUrls: [ETHIconUrl],
  isEip1559: true,
  rpcEndpoints: [{ url: 'https://goerli.infura.io/v3/' }, { url: 'wss://goerli.infura.io/ws/v3/' }],
  symbol: 'ETH',
  symbolName: 'Ethereum'
}

export const mockSepolia: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['https://sepolia.etherscan.io'],
  chainId: BraveWallet.SEPOLIA_CHAIN_ID,
  chainName: 'Sepolia Test Network',
  coin: 60,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
  decimals: 18,
  iconUrls: [ETHIconUrl],
  isEip1559: true,
  rpcEndpoints: [{ url: 'https://sepolia-infura.brave.com' }],
  symbol: 'ETH',
  symbolName: 'Ethereum'
}

export const mockEthLocalhost: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['http://localhost:7545/'],
  chainId: BraveWallet.LOCALHOST_CHAIN_ID,
  chainName: 'Localhost',
  coin: 60,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
  decimals: 18,
  iconUrls: [ETHIconUrl],
  isEip1559: false,
  rpcEndpoints: [{ url: 'http://localhost:7545/' }],
  symbol: 'ETH',
  symbolName: 'Ethereum'
}

export const mockFilecoinMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 'f',
  chainName: 'Filecoin Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://calibration.node.glif.io/rpc/v0' }],
  blockExplorerUrls: ['https://filscan.io/tipset/message-detail'],
  symbol: 'FIL',
  symbolName: 'Filecoin',
  decimals: 18,
  iconUrls: [FILECOINIconUrl],
  coin: BraveWallet.CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoin],
  isEip1559: false
}

export const mockFilecoinTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 't',
  chainName: 'Filecoin Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://mainnet-beta-solana.brave.com/rpc' }],
  blockExplorerUrls: ['https://calibration.filscan.io/tipset/message-detail'],
  symbol: 'FIL',
  symbolName: 'Filecoin',
  decimals: 18,
  iconUrls: [FILECOINIconUrl],
  coin: BraveWallet.CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoinTestnet],
  isEip1559: false
}

export const mockSolanaMainnetNetwork: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['https://explorer.solana.com'],
  chainId: '0x65',
  chainName: 'Solana Mainnet Beta',
  coin: BraveWallet.CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana],
  decimals: 9,
  iconUrls: [SOLIconUrl],
  isEip1559: false,
  rpcEndpoints: [{ url: 'https://api.testnet.solana.com' }],
  symbol: 'SOL',
  symbolName: 'Solana'
}

export const mockSolanaTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x66',
  chainName: 'Solana Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.testnet.solana.com' }],
  blockExplorerUrls: ['https://explorer.solana.com?cluster=testnet'],
  symbol: 'SOL',
  symbolName: 'Solana',
  decimals: 9,
  iconUrls: [SOLIconUrl],
  coin: BraveWallet.CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana],
  isEip1559: false
}

export const mockBitcoinMainnet: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['https://bitcoin.explorer'],
  chainId: 'bitcoin_mainnet',
  chainName: 'Bitcoin Mainnnet',
  coin: BraveWallet.CoinType.BTC,
  supportedKeyrings: [BraveWallet.KeyringId.kBitcoin84],
  decimals: 8,
  iconUrls: [BTCIconUrl],
  isEip1559: false,
  rpcEndpoints: [{ url: 'https://bitcoin.rpc' }],
  symbol: 'BTC',
  symbolName: 'Bitcoin'
}

export const mockBitcoinTestnet: BraveWallet.NetworkInfo = {
  activeRpcEndpointIndex: 0,
  blockExplorerUrls: ['https://bitcoin.explorer'],
  chainId: 'bitcoin_testnet',
  chainName: 'Bitcoin Testnet',
  coin: BraveWallet.CoinType.BTC,
  supportedKeyrings: [BraveWallet.KeyringId.kBitcoin84Testnet],
  decimals: 8,
  iconUrls: [BTCIconUrl],
  isEip1559: false,
  rpcEndpoints: [{ url: 'https://bitcoin.rpc/test' }],
  symbol: 'BTC',
  symbolName: 'Bitcoin'
}

export const mockNetworks: BraveWallet.NetworkInfo[] = [
  mockEthMainnet,
  mockGoerli,
  mockSepolia,
  mockFilecoinMainnetNetwork,
  mockFilecoinTestnetNetwork,
  mockSolanaMainnetNetwork,
  mockSolanaTestnetNetwork,
  mockEthLocalhost,
  mockBitcoinMainnet
]
