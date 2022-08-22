import { BraveWallet } from '../constants/types'
import { getLocale } from '../../common/locale'
import AllNetworksIcon from '../assets/svg-icons/all-networks-icon.svg'

export const AllNetworksOption: BraveWallet.NetworkInfo = {
  blockExplorerUrls: [],
  chainId: 'all',
  chainName: getLocale('braveWalletNetworkFilterAll'),
  coin: 0,
  decimals: 0,
  iconUrls: [AllNetworksIcon],
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [],
  symbol: 'all',
  symbolName: 'all',
  isEip1559: false
}

export const SupportedTopLevelChainIds = [
  BraveWallet.MAINNET_CHAIN_ID,
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.FILECOIN_MAINNET
]
