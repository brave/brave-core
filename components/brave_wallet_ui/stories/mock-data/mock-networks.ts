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
    chainId: '0x5',
    chainName: 'Ethereum Testnet Goerli',
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [{ url: 'https://goerli.infura.io/v3/' }, { url: 'wss://goerli.infura.io/ws/v3/' }],
    blockExplorerUrls: ['https://goerli.etherscan.io'],
    symbol: 'GOE',
    symbolName: 'Goerli Ether',
    decimals: 18,
    iconUrls: [ETHIconUrl],
    coin: BraveWallet.CoinType.ETH,
    isEip1559: true
  }
]
