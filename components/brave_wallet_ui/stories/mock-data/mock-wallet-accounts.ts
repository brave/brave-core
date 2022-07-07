import { BraveWallet, WalletAccountType } from '../../constants/types'

export const mockAccounts: WalletAccountType[] = [
  {
    id: '1',
    name: 'Account 1',
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    nativeBalanceRegistry: {
      '0x1': '311780000000000000'
    },
    accountType: 'Primary',
    tokenBalanceRegistry: {},
    coin: BraveWallet.CoinType.ETH,
    keyringId: undefined
  },
  {
    id: '2',
    name: 'Account 2',
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
    nativeBalanceRegistry: {
      '0x1': '311780000000000000'
    },
    accountType: 'Primary',
    tokenBalanceRegistry: {},
    coin: BraveWallet.CoinType.ETH,
    keyringId: undefined
  },
  {
    id: '3',
    name: 'Account 3',
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    nativeBalanceRegistry: {
      '0x1': '311780000000000000'
    },
    accountType: 'Primary',
    tokenBalanceRegistry: {},
    coin: BraveWallet.CoinType.ETH,
    keyringId: undefined
  }
]

export const mockedTransactionAccounts: WalletAccountType[] = [
  {
    id: '1',
    name: 'Account 1',
    address: '1',
    nativeBalanceRegistry: {
      '0x1': '311780000000000000'
    },
    accountType: 'Primary',
    tokenBalanceRegistry: {},
    coin: BraveWallet.CoinType.ETH,
    keyringId: undefined
  }
]
