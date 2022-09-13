import {
  getNetworkInfo,
  emptyNetwork,
  reduceNetworkDisplayName,
  getNetworksByCoinType,
  getTokensNetwork,
  getTokensCoinType,
  getCoinFromTxDataUnion,
  getFilecoinKeyringIdFromNetwork
} from './network-utils'
import { mockNetworks } from '../stories/mock-data/mock-networks'
import { BraveWallet } from '../constants/types'
import { mockNewAssetOptions } from '../stories/mock-data/mock-asset-options'

const ethToken = mockNewAssetOptions[0]
const bnbToken = mockNewAssetOptions[2]
const ethMainNetwork = mockNetworks[0]

describe('getNetworkInfo', () => {
  it('should return network info', () => {
    const { chainId, coin } = ethMainNetwork
    expect(getNetworkInfo(chainId, coin, mockNetworks)).toEqual(ethMainNetwork)
  })

  it('should return network object with default values if network with chainId is not found', () => {
    const chainId = 'fakeChainId'
    const coin = BraveWallet.CoinType.ETH
    expect(getNetworkInfo(chainId, coin, mockNetworks)).toEqual(emptyNetwork)
  })
})

describe('reduceNetworkDisplayName', () => {
  it('should return empty string if network name is falsy', () => {
    expect(reduceNetworkDisplayName('')).toEqual('')
  })

  it('should return first word when network name contains spaces', () => {
    const result = reduceNetworkDisplayName('Ethereum Mainet')
    expect(result).toEqual('Ethereum')
  })

  it('should concatenate first six character of network name and ..', () => {
    const networkName = 'TestNetworkName'
    const expected = 'TestNe..'
    expect(reduceNetworkDisplayName(networkName)).toBe(expected)
  })
})

describe('getNetworksByCoinType', () => {
  it('CoinType ETH, should return all ETH networks', () => {
    expect(getNetworksByCoinType(mockNetworks, BraveWallet.CoinType.ETH)).toEqual(mockNetworks)
  })
  it('CoinType random number, should return an empty array', () => {
    expect(getNetworksByCoinType(mockNetworks, 3000)).toEqual([])
  })
})

describe('getTokensNetwork', () => {
  it('Ethereum with chainId 0x1, should return ETH Mainnet info', () => {
    expect(getTokensNetwork(mockNetworks, ethToken)).toEqual(ethMainNetwork)
  })
  it('Binance Coin with chainId 0x3, should return ETH Ropsten Testnetwork info', () => {
    expect(getTokensNetwork(mockNetworks, bnbToken)).toEqual(mockNetworks[1])
  })
})

describe('getTokensCoinType', () => {
  it('Ethereum with chainId 0x1, should return CoinType ETH', () => {
    expect(getTokensCoinType(mockNetworks, ethToken)).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Binance Coin with chainId 0x3, should return ETH CoinType ETH', () => {
    expect(getTokensCoinType(mockNetworks, bnbToken)).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Binance Coin with chainId 0x3333333458, should default to CoinType ETH', () => {
    expect(getTokensCoinType(mockNetworks, { ...bnbToken, chainId: '0x3333333458' })).toEqual(BraveWallet.CoinType.ETH)
  })
})

describe('getCoinFromTxDataUnion', () => {
  it('Filecoin transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: {} as BraveWallet.FilTxData,
      ethTxData: undefined,
      ethTxData1559: undefined,
      solanaTxData: undefined
    })).toEqual(BraveWallet.CoinType.FIL)
  })
  it('Ethereum transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: undefined,
      ethTxData: {} as BraveWallet.TxData,
      ethTxData1559: undefined,
      solanaTxData: undefined
    })).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Ethereum1559 transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: undefined,
      ethTxData: undefined,
      ethTxData1559: {} as BraveWallet.TxData1559,
      solanaTxData: undefined
    })).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Solana transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: undefined,
      ethTxData: undefined,
      ethTxData1559: undefined,
      solanaTxData: {} as BraveWallet.SolanaTxData
    })).toEqual(BraveWallet.CoinType.SOL)
  })
})

describe('getFilecoinKeyringIdFromNetwork', () => {
  it('Filecoin testnet', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.FILECOIN_TESTNET, coin: BraveWallet.CoinType.FIL } as BraveWallet.NetworkInfo)).toEqual(BraveWallet.FILECOIN_TESTNET_KEYRING_ID)
  })
  it('Filecoin localhost', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.LOCALHOST_CHAIN_ID, coin: BraveWallet.CoinType.FIL } as BraveWallet.NetworkInfo)).toEqual(BraveWallet.FILECOIN_TESTNET_KEYRING_ID)
  })
  it('Filecoin mainnet', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.FILECOIN_MAINNET, coin: BraveWallet.CoinType.FIL } as BraveWallet.NetworkInfo)).toEqual(BraveWallet.FILECOIN_KEYRING_ID)
  })
  it('Non filecoin', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.ROPSTEN_CHAIN_ID, coin: BraveWallet.CoinType.ETH } as BraveWallet.NetworkInfo)).toEqual(undefined)
  })
})
