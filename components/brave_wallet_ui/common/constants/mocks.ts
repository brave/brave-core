// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  MeldCountry,
  MeldCryptoCurrency,
  MeldFiatCurrency,
  MeldPaymentMethod,
  SerializableTransactionInfo,
} from '../../constants/types'
import type { TokenBalancesRegistry } from '../slices/entities/token-balance.entity'

// images
import {
  ETHIcon,
  FILECOINIcon,
  SOLIcon,
} from '../../assets/network_token_icons/network_token_icons'

// mocks
import {
  mockAlgorandErc20TokenId,
  mockBasicAttentionToken,
  mockBasicAttentionTokenId,
  mockBinanceCoinErc20TokenId,
  mockBitcoinErc20TokenId,
  mockDaiTokenId,
  mockSplBasicAttentionTokenId,
  mockSplNftId,
  mockSplUSDCoinId,
  mockUSDCoinId,
  mockZrxErc20TokenId,
} from '../../stories/mock-data/mock-asset-options'

type EIP1559SerializableTransactionInfo = SerializableTransactionInfo & {
  txDataUnion: { ethTxData1559: BraveWallet.TxData1559 }
}

export const getMockedTransactionInfo =
  (): EIP1559SerializableTransactionInfo => {
    return {
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      id: '1',
      fromAccountId: mockEthAccount.accountId,
      txHash: '',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            chainId: BraveWallet.MAINNET_CHAIN_ID,
            to: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
            value: '0x01706a99bf354000', // 103700000000000000 wei (0.1037 ETH)
            // data: new Uint8Array(0),
            data: [] as number[],
            nonce: '0x03',
            gasLimit: '0x5208', // 2100
            gasPrice: '0x22ecb25c00', // 150 Gwei
          },
          maxPriorityFeePerGas: '',
          maxFeePerGas: '',
        },
        ethTxData: {} as any,
        filTxData: undefined,
        solanaTxData: undefined,
        btcTxData: undefined,
        zecTxData: undefined,
      },
      txStatus: BraveWallet.TransactionStatus.Approved,
      txType: BraveWallet.TransactionType.Other,
      txParams: [],
      txArgs: [],
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: {
        originSpec: 'https://brave.com',
        eTldPlusOne: 'brave.com',
      },
      effectiveRecipient: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
      isRetriable: false,
      swapInfoDeprecated: undefined,
      swapInfo: undefined,
    }
  }

export const mockNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x1',
  chainName: 'Ethereum Main Net',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.node.glif.io/rpc/v0' }],
  blockExplorerUrls: ['https://etherscan.io'],
  symbol: 'ETH',
  symbolName: 'Ethereum',
  decimals: 18,
  iconUrls: [ETHIcon],
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
}

export const mockFilecoinEVMMMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x13a',
  chainName: 'Filecoin EVM Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.node.glif.io/rpc/v1' }],
  blockExplorerUrls: ['https://filfox.info/en/message'],
  symbol: 'FIL',
  symbolName: 'Filcoin',
  decimals: 18,
  iconUrls: [FILECOINIcon],
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
}

export const mockFilecoinEVMMTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x4cb2f',
  chainName: 'Filecoin EVM Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.calibration.node.glif.io/rpc/v1' }],
  blockExplorerUrls: ['https://calibration.filfox.info/en/message'],
  symbol: 'FIL',
  symbolName: 'Filcoin',
  decimals: 18,
  iconUrls: [FILECOINIcon],
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
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
  iconUrls: [FILECOINIcon],
  coin: BraveWallet.CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoin],
}

export const mockFilecoinTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 't',
  chainName: 'Filecoin Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://solana-mainnet.wallet.brave.com' }],
  blockExplorerUrls: ['https://calibration.filscan.io/tipset/message-detail'],
  symbol: 'FIL',
  symbolName: 'Filecoin',
  decimals: 18,
  iconUrls: [FILECOINIcon],
  coin: BraveWallet.CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoinTestnet],
}

export const mockSolanaMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x65',
  chainName: 'Solana Mainnet Beta',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.testnet.solana.com' }],
  blockExplorerUrls: ['https://explorer.solana.com'],
  symbol: 'SOL',
  symbolName: 'Solana',
  decimals: 9,
  iconUrls: [SOLIcon],
  coin: BraveWallet.CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana],
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
  iconUrls: [SOLIcon],
  coin: BraveWallet.CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana],
}

export const mockBtcMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 'bitcoin_mainnet',
  chainName: 'Bitcoin Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://bitcoin-mainnet.wallet.brave.com/' }],
  blockExplorerUrls: ['https://www.blockchain.com/explorer'],
  symbol: 'BTC',
  symbolName: 'Bitcoin',
  decimals: 8,
  iconUrls: [],
  coin: BraveWallet.CoinType.BTC,
  supportedKeyrings: [BraveWallet.KeyringId.kBitcoin84],
}

export const mockZecMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: BraveWallet.Z_CASH_MAINNET,
  chainName: 'Zcash Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://zec-mainnet.wallet.brave.com/' }],
  blockExplorerUrls: ['https://3xpl.com/zcash/transaction'],
  symbol: 'ZEC',
  symbolName: 'Zcash',
  decimals: 8,
  iconUrls: [],
  coin: BraveWallet.CoinType.ZEC,
  supportedKeyrings: [BraveWallet.KeyringId.kZCashMainnet],
}

export const mockZecTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: BraveWallet.Z_CASH_TESTNET,
  chainName: 'Zcash Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://zec-testnet.wallet.brave.com/' }],
  blockExplorerUrls: ['https://blockexplorer.one/zcash/testnet/tx'],
  symbol: 'ZEC',
  symbolName: 'Zcash',
  decimals: 8,
  iconUrls: [],
  coin: BraveWallet.CoinType.ZEC,
  supportedKeyrings: [BraveWallet.KeyringId.kZCashTestnet],
}

export const mockPolkadotMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: BraveWallet.POLKADOT_MAINNET,
  chainName: 'Polkadot Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://polkadot-mainnet.wallet.brave.com/' }],
  blockExplorerUrls: ['https://polkadot.subscan.io'],
  symbol: 'DOT',
  symbolName: 'Polkadot',
  decimals: 10,
  iconUrls: [],
  coin: BraveWallet.CoinType.DOT,
  supportedKeyrings: [BraveWallet.KeyringId.kPolkadotMainnet],
}

export const mockAccount: BraveWallet.AccountInfo = {
  name: 'mockAccountName',
  address: '0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0',
  accountId: {
    coin: BraveWallet.CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0',
    accountIndex: 0,
    uniqueKey: 'unique_key_0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0',
  },
  hardware: undefined,
}

export const mockBtcAccount: BraveWallet.AccountInfo = {
  name: 'mockBtcAccountName',
  address: 'bc1q4500000000000000000',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84,
    kind: BraveWallet.AccountKind.kDerived,
    address: 'bc1q4500000000000000000',
    accountIndex: 0,
    uniqueKey: 'unique_key_bc1q4500000000000000000',
  },
  hardware: undefined,
}

export const mockZecAccount: BraveWallet.AccountInfo = {
  name: 'mockZecAccountName',
  address: 'zCash-address',
  accountId: {
    coin: BraveWallet.CoinType.ZEC,
    keyringId: BraveWallet.KeyringId.kZCashMainnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 'zCash-address',
    accountIndex: 0,
    uniqueKey: 'unique_key_zCash-address',
  },
  hardware: undefined,
}

export const mockEthAccount: BraveWallet.AccountInfo = {
  hardware: undefined,
  name: 'mockEthAccountName',
  address: '0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db',
  accountId: {
    coin: BraveWallet.CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db',
    accountIndex: 0,
    uniqueKey: 'unique_key_0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db',
  },
}

export const mockSolanaAccount: BraveWallet.AccountInfo = {
  name: 'MockSolanaAccount',
  address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRez',
  accountId: {
    coin: BraveWallet.CoinType.SOL,
    keyringId: BraveWallet.KeyringId.kSolana,
    kind: BraveWallet.AccountKind.kDerived,
    address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRez',
    accountIndex: 0,
    uniqueKey: 'unique_key_5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRez',
  },
  hardware: undefined,
}

export const mockFilecoinAccount: BraveWallet.AccountInfo = {
  name: 'MockFilecoinAccount',
  address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
  accountId: {
    coin: BraveWallet.CoinType.FIL,
    keyringId: BraveWallet.KeyringId.kFilecoinTestnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
    accountIndex: 0,
    uniqueKey: 'unique_key_t1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
  },
  hardware: undefined,
}

export const mockBitcoinAccount: BraveWallet.AccountInfo = {
  name: 'MockBitcoinAccount',
  address: '',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: 'unique_key_MockBitcoinAccount',
  },
  hardware: undefined,
}

export const mockBitcoinTestnetAccount: BraveWallet.AccountInfo = {
  name: 'MockBitcoinTestnetAccount',
  address: '',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84Testnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: 'unique_key_MockBitcoinTestnetAccount',
  },
  hardware: undefined,
}

export const mockCardanoAccount: BraveWallet.AccountInfo = {
  name: 'mockAdaAccountName',
  address: '',
  accountId: {
    coin: BraveWallet.CoinType.ADA,
    keyringId: BraveWallet.KeyringId.kCardanoMainnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: '1_0_0_0',
  },
  hardware: undefined,
}

export const mockSpotPriceRegistry: BraveWallet.AssetPrice[] = [
  {
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1',
    address: '0x0000000000000000000000000000000000000000',
    price: '4000',
    percentageChange24h: 'mockValue',
    vsCurrency: 'USD',
    cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
    source: BraveWallet.AssetPriceSource.kCoingecko,
  },
  {
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1',
    address: '0x0000000000000000000000000000000000000000',
    price: '100',
    percentageChange24h: 'mockValue',
    vsCurrency: 'USD',
    cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
    source: BraveWallet.AssetPriceSource.kCoingecko,
  },
  {
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1',
    address: mockBasicAttentionToken.contractAddress,
    price: '0.88',
    percentageChange24h: 'mockValue',
    vsCurrency: 'USD',
    cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
    source: BraveWallet.AssetPriceSource.kCoingecko,
  },
]

export const mockAddresses: string[] = [
  '0xea674fdde714fd979de3edf0f56aa9716b898ec8',
  '0xdbf41e98f541f19bb044e604d2520f3893eefc79',
  '0xcee177039c99d03a6f74e95bbba2923ceea43ea2',
]

export const mockFilAddresses: string[] = [
  't1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  'f1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  't3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
]

export const mockFilInvalilAddresses: string[] = [
  '',
  't1lqarsh4nkg545ilaoqdsbtj4uofplt6sto2ziy',
  'f1lqarsh4nkg545ilaoqdsbtj4uofplt6sto2f6ziy',
  't3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3ju3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jfpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'a1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  'b1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  'c3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'd3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
]

export const mockSolDappSignTransactionRequest: //
BraveWallet.SignSolTransactionsRequest = {
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app',
  },
  id: 0,
  fromAccountId: mockSolanaAccount.accountId,
  txDatas: [
    {
      recentBlockhash: 'B7Kg79jDm48LMdB4JB2hu82Yfsuz5xYm2cQDBYmKdDSn',
      lastValidBlockHeight: 0 as unknown as bigint,
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: 0 as unknown as bigint,
      amount: 0 as unknown as bigint,
      txType: 12,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: undefined,
        },
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1,
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111',
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined,
    },
  ],
  rawMessages: [[1, 2, 3]],
  chainId: {
    chainId: BraveWallet.SOLANA_MAINNET,
    coin: BraveWallet.CoinType.SOL,
  },
}

// BraveWallet.TransactionInfo (selectedPendingTransaction)
export const mockSolDappSignAndSendTransactionRequest: //
SerializableTransactionInfo = {
  chainId: '0x67',
  id: 'e1eae32d-5bc2-40ac-85e5-2a4a5fbe8a5f',
  fromAccountId: mockSolanaAccount.accountId,
  txHash: '',
  txDataUnion: {
    ethTxData: undefined,
    ethTxData1559: undefined,
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined,
    solanaTxData: {
      recentBlockhash: 'C115cyMDVoGGYNd4r8vFy5qPJEUdoJQQCXMYYKQTQimn',
      lastValidBlockHeight: '0',
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: '0',
      amount: '0',
      txType: 11,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: { val: 1 },
              isSigner: true,
              isWritable: true,
            },
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: {
            accountParams: [
              {
                localizedName: 'From',
                name: BraveWallet.FROM_ACCOUNT,
              },
              {
                localizedName: 'To',
                name: BraveWallet.TO_ACCOUNT,
              },
            ],
            instructionType: BraveWallet.SolanaSystemInstruction.kTransfer,
            params: [
              {
                localizedName: 'lamports',
                name: BraveWallet.LAMPORTS,
                value: '1',
                type: BraveWallet.SolanaInstructionParamType.kUint64,
              },
            ],
          },
        },
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1,
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111',
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined,
    },
  },
  txStatus: 0,
  txType: 11,
  txParams: [],
  txArgs: [],
  createdTime: { microseconds: 1654540245386000 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app',
  },
  effectiveRecipient: undefined,
  isRetriable: false,
  swapInfoDeprecated: undefined,
  swapInfo: undefined,
}

export const mockSolDappSignAllTransactionsRequest: //
BraveWallet.SignSolTransactionsRequest = {
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app',
  },
  id: 3,
  fromAccountId: mockSolanaAccount.accountId,
  txDatas: [
    {
      recentBlockhash: '8Yq6DGZBh9oEJsCVhUjTqN9kPiLoeYJ7J4n9TnpPYjqW',
      lastValidBlockHeight: 0 as unknown as bigint,
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: 0 as unknown as bigint,
      amount: 0 as unknown as bigint,
      txType: 12,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: undefined,
        },
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1,
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111',
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined,
    },
    {
      recentBlockhash: '8Yq6DGZBh9oEJsCVhUjTqN9kPiLoeYJ7J4n9TnpPYjqW',
      lastValidBlockHeight: 0 as unknown as bigint,
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: 0 as unknown as bigint,
      amount: 0 as unknown as bigint,
      txType: 12,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: undefined,
        },
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1,
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111',
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined,
    },
  ],
  rawMessages: [[1]],
  chainId: {
    chainId: BraveWallet.SOLANA_MAINNET,
    coin: BraveWallet.CoinType.SOL,
  },
}

export const mockSignCardanoTransactionRequest: //
BraveWallet.SignCardanoTransactionRequest = {
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app',
  },
  id: 3,
  accountId: mockCardanoAccount.accountId,
  chainId: {
    chainId: BraveWallet.CARDANO_MAINNET,
    coin: BraveWallet.CoinType.ADA,
  },
  rawTxData:
    '84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a'
    + 'dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d'
    + '9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac'
    + 'e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a'
    + 'd9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1'
    + 'c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776'
    + 'f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b'
    + '6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62'
    + 'e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6',
  detailsJson: JSON.stringify({
    inputs: [
      {
        txHash:
          'a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925',
        index: '0',
        value: '10000000',
        tokens: [],
        address:
          'addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zzmj'
          + '4z53l7lh5u7z08l0rvp49ht88s5uskl6tsl',
      },
    ],
    outputs: [
      {
        address:
          'addr1qy8ampwn98c9y6gcea9cturevw2njm9pcs8apt8aunljymuy88wktx2awyus6hc'
          + 'cdu76k3n6gjvkvl3zunyc47k4tgns5k87m6',
        value: '10000000',
        tokens: [],
      },
    ],
  }),
}

export const mockOnRampCurrency: BraveWallet.OnRampCurrency = {
  currencyCode: 'USD',
  currencyName: 'United States Dollar',
  providers: [],
}

export const mockOnRampCurrencies: BraveWallet.OnRampCurrency[] = [
  mockOnRampCurrency,
  {
    currencyCode: 'EUR',
    currencyName: 'Euro',
    providers: [],
  },
  {
    currencyCode: 'GBP',
    currencyName: 'British Pound Sterling',
    providers: [],
  },
]

export type NativeAssetBalanceRegistry = Record<
  string, // account address
  | Record<
      string, // chainId
      string // balance
    >
  | undefined
>

export const mockNativeBalanceRegistry: NativeAssetBalanceRegistry = {
  [mockAccount.address]: {
    [BraveWallet.BITCOIN_MAINNET]: '0',
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '836',
    [BraveWallet.FILECOIN_MAINNET]: '0',
    [BraveWallet.MAINNET_CHAIN_ID]: '12312',
    [BraveWallet.SOLANA_MAINNET]: '0',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '2322',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '45100002',
    [BraveWallet.AURORA_MAINNET_CHAIN_ID]: '4326',
    [BraveWallet.AVALANCHE_MAINNET_CHAIN_ID]: '345',
    [BraveWallet.BASE_MAINNET_CHAIN_ID]: '56453455',
    [BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID]: '444',
    [BraveWallet.CELO_MAINNET_CHAIN_ID]: '55851',
    [BraveWallet.FANTOM_MAINNET_CHAIN_ID]: '1',
    [BraveWallet.GNOSIS_CHAIN_ID]: '440502',
    [BraveWallet.NEON_EVM_MAINNET_CHAIN_ID]: '222',
    [BraveWallet.OPTIMISM_MAINNET_CHAIN_ID]: '567',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '111',
    [BraveWallet.POLYGON_ZKEVM_CHAIN_ID]: '98094343',
    [BraveWallet.ZK_SYNC_ERA_CHAIN_ID]: '2621',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '0',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '7798',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '777',
    [BraveWallet.PALM_CHAIN_ID]: '2',
  },
  [mockEthAccount.address]: {
    [BraveWallet.BITCOIN_MAINNET]: '0',
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '22',
    [BraveWallet.FILECOIN_MAINNET]: '3111',
    [BraveWallet.MAINNET_CHAIN_ID]: '33214',
    [BraveWallet.SOLANA_MAINNET]: '0',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '1221',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '251002',
    [BraveWallet.AURORA_MAINNET_CHAIN_ID]: '1111',
    [BraveWallet.AVALANCHE_MAINNET_CHAIN_ID]: '565',
    [BraveWallet.BASE_MAINNET_CHAIN_ID]: '4444',
    [BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID]: '2122',
    [BraveWallet.CELO_MAINNET_CHAIN_ID]: '1',
    [BraveWallet.FANTOM_MAINNET_CHAIN_ID]: '0',
    [BraveWallet.GNOSIS_CHAIN_ID]: '2',
    [BraveWallet.NEON_EVM_MAINNET_CHAIN_ID]: '0',
    [BraveWallet.OPTIMISM_MAINNET_CHAIN_ID]: '2',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '55',
    [BraveWallet.POLYGON_ZKEVM_CHAIN_ID]: '666',
    [BraveWallet.ZK_SYNC_ERA_CHAIN_ID]: '5377',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '1',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '9',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '727',
    [BraveWallet.PALM_CHAIN_ID]: '1',
  },
  [mockSolanaAccount.address]: {
    [BraveWallet.SOLANA_MAINNET]: '7432',
  },
  [mockSolanaAccount.address]: {
    [BraveWallet.SOLANA_MAINNET]: '45434545435',
  },
  [mockFilecoinAccount.address]: {
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '34598722',
    [BraveWallet.FILECOIN_MAINNET]: '345545',
    [BraveWallet.MAINNET_CHAIN_ID]: '1000',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '1000',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '3000',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '330',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '220',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '5550',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '40',
    [BraveWallet.PALM_CHAIN_ID]: '70',
  },
  [mockFilecoinAccount.address]: {
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '2334',
    [BraveWallet.FILECOIN_MAINNET]: '35',
    [BraveWallet.MAINNET_CHAIN_ID]: '220',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '600',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '400',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '30',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '20',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '50',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '4',
    [BraveWallet.PALM_CHAIN_ID]: '7',
  },
}

export const mockTokenBalanceRegistry: TokenBalancesRegistry = {
  accounts: {
    [mockAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.MAINNET_CHAIN_ID]: {
          tokenBalances: {
            [mockBasicAttentionTokenId]: '111',
            [mockBinanceCoinErc20TokenId]: '222',
            [mockBitcoinErc20TokenId]: '333',
            [mockAlgorandErc20TokenId]: '444',
            [mockZrxErc20TokenId]: '555',
            [mockDaiTokenId]: '666',
            [mockUSDCoinId]: '777',
          },
        },
      },
    },
    [mockEthAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.MAINNET_CHAIN_ID]: {
          tokenBalances: {
            [mockBasicAttentionTokenId]: '11',
            [mockBinanceCoinErc20TokenId]: '22',
            [mockBitcoinErc20TokenId]: '33',
            [mockAlgorandErc20TokenId]: '44',
            [mockZrxErc20TokenId]: '55',
            [mockDaiTokenId]: '66',
            [mockUSDCoinId]: '77',
          },
        },
      },
    },
    [mockSolanaAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.SOLANA_MAINNET]: {
          tokenBalances: {
            [mockSplNftId]: '1',
            [mockSplUSDCoinId]: '14444',
            [mockSplBasicAttentionTokenId]: '99999',
          },
        },
      },
    },
    [mockFilecoinAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.FILECOIN_MAINNET]: {
          tokenBalances: {},
        },
      },
    },
  },
}

export const mockMeldFiatCurrency: MeldFiatCurrency = {
  currencyCode: 'USD',
  name: 'United States Dollar',
  symbolImageUrl: '',
}

export const mockMeldFiatCurrencies: MeldFiatCurrency[] = [
  {
    'currencyCode': 'AFN',
    'name': 'Afghani',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/AFN/symbol.png',
  },
  {
    'currencyCode': 'DZD',
    'name': 'Algerian Dinar',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/DZD/symbol.png',
  },
  {
    'currencyCode': 'ARS',
    'name': 'Argentine Peso',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/ARS/symbol.png',
  },
  {
    'currencyCode': 'AMD',
    'name': 'Armenian Dram',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/AMD/symbol.png',
  },
  {
    'currencyCode': 'AWG',
    'name': 'Aruban Florin',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/AWG/symbol.png',
  },
  {
    'currencyCode': 'AUD',
    'name': 'Australian Dollar',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/AUD/symbol.png',
  },
  {
    'currencyCode': 'AZN',
    'name': 'Azerbaijan Manat',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/AZN/symbol.png',
  },
  {
    'currencyCode': 'BSD',
    'name': 'Bahamian Dollar',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/BSD/symbol.png',
  },
  {
    'currencyCode': 'BHD',
    'name': 'Bahraini Dinar',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/BHD/symbol.png',
  },
  {
    'currencyCode': 'THB',
    'name': 'Baht',
    'symbolImageUrl': 'https://images-currency.meld.io/fiat/THB/symbol.png',
  },
]

export const mockMeldCryptoCurrencies: MeldCryptoCurrency[] = [
  {
    'currencyCode': '00',
    'name': '00 Token',
    'chainCode': 'ETH',
    'chainName': 'Ethereum',
    'chainId': '0x1',
    'contractAddress': undefined,
    'symbolImageUrl': 'https://images-currency.meld.io/crypto/00/symbol.png',
  },
  {
    'currencyCode': 'ZRX',
    'name': '0x',
    'chainCode': 'ETH',
    'chainName': 'Ethereum',
    'chainId': '0x1',
    'contractAddress': '0xe41d2489571d322189246dafa5ebde1f4699f498',
    'symbolImageUrl': 'https://images-currency.meld.io/crypto/ZRX/symbol.png',
  },
  {
    'currencyCode': 'OXD_FTM',
    'name': '0xDAO',
    'chainCode': 'FTM',
    'chainName': 'Fantom',
    'chainId': '0xfa',
    'contractAddress': undefined,
    'symbolImageUrl':
      'https://images-currency.meld.io/crypto/OXD_FTM/symbol.png',
  },
  {
    'currencyCode': '1INCH',
    'name': '1inch',
    'chainCode': 'ETH',
    'chainName': 'Ethereum',
    'chainId': '0x1',
    'contractAddress': '0x111111111117dc0aa78b770fa6a738034120c302',
    'symbolImageUrl': 'https://images-currency.meld.io/crypto/1INCH/symbol.png',
  },
  {
    'currencyCode': '1INCH_BSC',
    'name': '1inch',
    'chainCode': 'BSC',
    'chainName': 'BNB Smart Chain',
    'chainId': '0x38',
    'contractAddress': '0x111111111117dc0aa78b770fa6a738034120c302',
    'symbolImageUrl':
      'https://images-currency.meld.io/crypto/1INCH_BSC/symbol.png',
  },
]

export const mockMeldCryptoQuotes = [
  {
    'transactionType': 'CRYPTO_PURCHASE',
    'exchangeRate': '3355.431',
    'transactionFee': '4.99',
    'sourceCurrencyCode': 'USD',
    'sourceAmount': '100',
    'sourceAmountWithoutFee': '93.23',
    'fiatAmountWithoutFees': '93.23',
    'totalFee': '6.77',
    'networkFee': '0.78',
    'paymentMethod': 'CREDIT_DEBIT_CARD',
    'destinationCurrencyCode': 'ETH',
    'destinationAmount': '0.02980243',
    'destinationAmountWithoutFees': undefined,
    'customerScore': '20.0',
    'serviceProvider': 'TRANSAK',
    'countryCode': 'US',
  },
]

export const mockServiceProviders = [
  {
    'name': 'Transak',
    'serviceProvider': 'TRANSAK',
    'status': 'LIVE',
    'webSiteUrl': 'https://transak.com',
    'categories': ['CRYPTO_OFFRAMP', 'CRYPTO_ONRAMP'],
    'categoryStatuses': {
      'CRYPTO_OFFRAMP': 'LIVE',
      'CRYPTO_ONRAMP': 'LIVE',
    },
    'logoImages': {
      'darkUrl': 'https://images-serviceprovider.meld.io/TRANSAK/logo_dark.png',
      'darkShortUrl':
        'https://images-serviceprovider.meld.io/TRANSAK/short_logo_dark.png',
      'lightUrl':
        'https://images-serviceprovider.meld.io/TRANSAK/logo_light.png',
      'lightShortUrl':
        'https://images-serviceprovider.meld.io/TRANSAK/short_logo_light.png',
    },
  },
]

export const mockMeldCountries = [
  {
    'countryCode': 'AF',
    'name': 'Afghanistan',
    'flagImageUrl': 'https://images-country.meld.io/AF/flag.svg',
    'regions': null,
  },
  {
    'countryCode': 'AL',
    'name': 'Albania',
    'flagImageUrl': 'https://images-country.meld.io/AL/flag.svg',
    'regions': null,
  },
  {
    'countryCode': 'DZ',
    'name': 'Algeria',
    'flagImageUrl': 'https://images-country.meld.io/DZ/flag.svg',
    'regions': null,
  },
  {
    'countryCode': 'AS',
    'name': 'American Samoa',
    'flagImageUrl': 'https://images-country.meld.io/AS/flag.svg',
    'regions': null,
  },
  {
    'countryCode': 'AD',
    'name': 'Andorra',
    'flagImageUrl': 'https://images-country.meld.io/AD/flag.svg',
    'regions': null,
  },
] as unknown as MeldCountry[]

export const mockMeldPaymentMethods = [
  {
    'paymentMethod': 'APPLE_PAY',
    'name': 'Apple Pay',
    'paymentType': 'MOBILE_WALLET',
    'logoImages': {
      'darkUrl': 'https://images-paymentMethod.meld.io/APPLE_PAY/logo_dark.png',
      'darkShortUrl': null,
      'lightUrl':
        'https://images-paymentMethod.meld.io/APPLE_PAY/logo_light.png',
      'lightShortUrl': null,
    },
  },
  {
    'paymentMethod': 'CREDIT_DEBIT_CARD',
    'name': 'Credit & Debit Card',
    'paymentType': 'CARD',
    'logoImages': {
      'darkUrl':
        'https://images-paymentMethod.meld.io/CREDIT_DEBIT_CARD/logo_dark.png',
      'darkShortUrl': null,
      'lightUrl':
        'https://images-paymentMethod.meld.io/CREDIT_DEBIT_CARD/logo_light.png',
      'lightShortUrl': null,
    },
  },
  {
    'paymentMethod': 'FAST',
    'name': 'FAST',
    'paymentType': 'BANK_TRANSFER',
    'logoImages': {
      'darkUrl': 'https://images-paymentMethod.meld.io/FAST/logo_dark.png',
      'darkShortUrl': null,
      'lightUrl': 'https://images-paymentMethod.meld.io/FAST/logo_light.png',
      'lightShortUrl': null,
    },
  },
  {
    'paymentMethod': 'NG_BANK_TRANSFER',
    'name': 'Local Manual Bank Transfer',
    'paymentType': 'BANK_TRANSFER',
    'logoImages': {
      'darkUrl':
        'https://images-paymentMethod.meld.io/NG_BANK_TRANSFER/logo_dark.png',
      'darkShortUrl': null,
      'lightUrl':
        'https://images-paymentMethod.meld.io/NG_BANK_TRANSFER/logo_light.png',
      'lightShortUrl': null,
    },
  },
  {
    'paymentMethod': 'SEPA',
    'name': 'SEPA',
    'paymentType': 'BANK_TRANSFER',
    'logoImages': {
      'darkUrl': 'https://images-paymentMethod.meld.io/SEPA/logo_dark.png',
      'darkShortUrl': null,
      'lightUrl': 'https://images-paymentMethod.meld.io/SEPA/logo_light.png',
      'lightShortUrl': null,
    },
  },
  {
    'paymentMethod': 'SPEI',
    'name': 'SPEI',
    'paymentType': 'BANK_TRANSFER',
    'logoImages': {
      'darkUrl': 'https://images-paymentMethod.meld.io/SPEI/logo_dark.png',
      'darkShortUrl': null,
      'lightUrl': 'https://images-paymentMethod.meld.io/SPEI/logo_light.png',
      'lightShortUrl': null,
    },
  },
] as unknown as MeldPaymentMethod[]
