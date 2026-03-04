// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// utils
import { buildExplorerUrl } from './block-explorer-utils'

// mocks
import {
  mockNetwork,
  mockFilecoinMainnetNetwork,
  mockFilecoinTestnetNetwork,
  mockSolanaTestnetNetwork,
  mockSolanaMainnetNetwork,
  mockBtcMainnetNetwork,
  mockZecMainnetNetwork,
  mockZecTestnetNetwork,
} from '../common/constants/mocks'

const ethereumTransactionId =
  '0xc0b00b52d0e4ea19d81f51730adc79a1850ce4c08c38e630b0c6668760d264c0'
const fileCointTransactionId =
  'bafy2bzacec7altnqomkd6mu2kqrfgfontjfsg67xjv6zs3vn3wakumhj3bttg'
const solanaTransacitonId =
  'wi14VnU8msHsmNq4N9j7YAWu7jdDPUxMKKM2VLcstSevKv4NfKbxNEUtMquSoMjtZUMd4YSASzmwvDamkWTfh9b'
const bitcoinTransactionId =
  '7b4c1acbb7e6510ee9473db0ecf8bf4a99eb246dad0b9aa1be4a991e0bd36fc2'
const bitcoinAddress = 'bc1qlcqtaqx8qnacsgv4z3uv0dhlvrl066lh8secet'
const zcashTransactionId =
  '3bc513afc84befb9774f667eb4e63266a7229ab1fdb43476dd7c3a33d16b3101'
const zcashAddress = 't1UYsZVJkLPeMjxEtACvSxfWuNmddpWaqK7'

it('ethereum explorer url', () => {
  const assertion = buildExplorerUrl(
    mockNetwork,
    'tx',
    ethereumTransactionId,
    undefined,
  )
  expect(assertion).toEqual(`https://etherscan.io/tx/${ethereumTransactionId}`)
})

it('filecoin mainnet explorer url', () => {
  const assertion = buildExplorerUrl(
    mockFilecoinMainnetNetwork,
    'tx',
    fileCointTransactionId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://filscan.io/tipset/message-detail?cid=${fileCointTransactionId}`,
  )
})

it('filecoin test explorer url', () => {
  const assertion = buildExplorerUrl(
    mockFilecoinTestnetNetwork,
    'tx',
    fileCointTransactionId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://calibration.filscan.io/tipset/message-detail?cid=${fileCointTransactionId}`,
  )
})

it('solana test explorer url', () => {
  const assertion = buildExplorerUrl(
    mockSolanaTestnetNetwork,
    'tx',
    solanaTransacitonId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://explorer.solana.com/tx/${solanaTransacitonId}?cluster=testnet`,
  )
})

it('solana mainnet explorer url', () => {
  const assertion = buildExplorerUrl(
    mockSolanaMainnetNetwork,
    'tx',
    solanaTransacitonId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://explorer.solana.com/tx/${solanaTransacitonId}`,
  )
})

it('btc mainnet explorer url for tx', () => {
  const assertion = buildExplorerUrl(
    mockBtcMainnetNetwork,
    'tx',
    bitcoinTransactionId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://www.blockchain.com/explorer/transactions/btc/${bitcoinTransactionId}`,
  )
})

it('btc mainnet explorer url for address', () => {
  const assertion = buildExplorerUrl(
    mockBtcMainnetNetwork,
    'address',
    bitcoinAddress,
    undefined,
  )
  expect(assertion).toEqual(
    `https://www.blockchain.com/explorer/addresses/btc/${bitcoinAddress}`,
  )
})

it('btc mainnet explorer url for unsupported type', () => {
  const assertion = buildExplorerUrl(
    mockBtcMainnetNetwork,
    'token',
    bitcoinAddress,
    undefined,
  )
  expect(assertion).toEqual(
    `https://www.blockchain.com/explorer/search?search=${bitcoinAddress}`,
  )
})

it('zec mainnet explorer url for tx', () => {
  const assertion = buildExplorerUrl(
    mockZecMainnetNetwork,
    'tx',
    zcashTransactionId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://3xpl.com/zcash/transaction/${zcashTransactionId}`,
  )
})

it('zec mainnet explorer url for address', () => {
  const assertion = buildExplorerUrl(
    mockZecMainnetNetwork,
    'address',
    zcashAddress,
    undefined,
  )
  expect(assertion).toEqual(`https://3xpl.com/zcash/address/${zcashAddress}`)
})

it('zec testnet explorer url for tx', () => {
  const assertion = buildExplorerUrl(
    mockZecTestnetNetwork,
    'tx',
    zcashTransactionId,
    undefined,
  )
  expect(assertion).toEqual(
    `https://blockexplorer.one/zcash/testnet/tx/${zcashTransactionId}`,
  )
})

it('zec testnet explorer url for address', () => {
  const assertion = buildExplorerUrl(
    mockZecTestnetNetwork,
    'address',
    zcashAddress,
    undefined,
  )
  expect(assertion).toEqual(
    `https://blockexplorer.one/zcash/testnet/address/${zcashAddress}`,
  )
})
