import { BraveWallet } from '../../constants/types'
import {
  ETHIconUrl
} from '../../assets/asset-icons'
export const mockNetworks: BraveWallet.NetworkInfo[] = [
  {
    chainId: '0x1',
    chainName: 'Ethereum Main Net',
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [{ url: 'https://mainnet.infura.io/v3/' }],
    blockExplorerUrls: ['https://etherscan.io', 'https://etherchain.org'],
    symbol: 'ETH',
    symbolName: 'Ethereum',
    decimals: 18,
    iconUrls: [ETHIconUrl],
    coin: BraveWallet.CoinType.ETH,
    isEip1559: true
  },
  {
    chainId: '0x3',
    chainName: 'Ethereum Testnet Ropsten',
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [{ url: 'https://ropsten.infura.io/v3/' }, { url: 'wss://ropsten.infura.io/ws/v3/' }],
    blockExplorerUrls: ['https://ropsten.etherscan.io'],
    symbol: 'ROP',
    symbolName: 'Ropsten Ether',
    decimals: 18,
    iconUrls: [ETHIconUrl],
    coin: BraveWallet.CoinType.ETH,
    isEip1559: true
  }
]
