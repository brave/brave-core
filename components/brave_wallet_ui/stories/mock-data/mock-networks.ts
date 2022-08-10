import { BraveWallet } from '../../constants/types'
import {
  ETHIconUrl
} from '../../assets/asset-icons'
export const mockNetworks: BraveWallet.NetworkInfo[] = [
  {
    chainId: '0x1',
    chainName: 'Ethereum Main Net',
    rpcUrls: ['https://mainnet.infura.io/v3/'],
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
    rpcUrls: ['https://ropsten.infura.io/v3/', 'wss://ropsten.infura.io/ws/v3/'],
    blockExplorerUrls: ['https://ropsten.etherscan.io'],
    symbol: 'ROP',
    symbolName: 'Ropsten Ether',
    decimals: 18,
    iconUrls: [ETHIconUrl],
    coin: BraveWallet.CoinType.ETH,
    isEip1559: true
  }
]
