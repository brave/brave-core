import { BraveWallet } from '../constants/types'
import { ETH } from '../options/asset-options'

export const GetTokenParam = (selectedNetwork: BraveWallet.EthereumChain, token: BraveWallet.ERCToken): string => {
  const isEthereumNetwork = selectedNetwork.chainId === BraveWallet.MAINNET_CHAIN_ID

  if (!isEthereumNetwork) {
    return token.symbol.toLowerCase()
  }

  return token.symbol.toLowerCase() === ETH.asset.symbol.toLowerCase()
    ? token.symbol.toLowerCase()
    : token.contractAddress
}
