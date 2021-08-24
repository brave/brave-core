import { EthereumChain } from '../../constants/types'
import {
  ETHIconUrl
} from '../../assets/asset-icons'
export const mockNetworks: EthereumChain[] = [
  {
    chainId: '0x1',
    chainName: 'Ethereum Main Net',
    rpcUrls: ['https://mainnet.infura.io/v3/'],
    blockExplorerUrls: ['https://etherscan.io/', 'https://etherchain.org/'],
    currency: {
      symbol: 'ETH',
      name: 'Ethereum',
      decimals: 18
    },
    iconUrls: [ETHIconUrl]
  }
]
