import { BraveWallet } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AllNetworksOption: BraveWallet.NetworkInfo = {
  blockExplorerUrls: [],
  chainId: 'all',
  chainName: getLocale('braveWalletNetworkFilterAll'),
  coin: 0,
  decimals: 0,
  iconUrls: [],
  rpcUrls: [],
  symbol: 'all',
  symbolName: 'all',
  data: {} as BraveWallet.NetworkInfoData
}

export const SupportedTopLevelChainIds = [
  BraveWallet.MAINNET_CHAIN_ID,
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.FILECOIN_MAINNET
]
