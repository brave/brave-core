import { BraveWallet, WalletAccountType, GetFlattenedAccountBalancesReturnInfo } from '../constants/types'
import { formatBalance } from './format-balances'
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

// This will get the sum balance for each token between all accounts
export const GetFlattenedAccountBalances = (accounts: WalletAccountType[]): GetFlattenedAccountBalancesReturnInfo[] => {
  if (accounts.length === 0) {
    return []
  }
  return accounts[0].tokens.map((token, index) => {
    return {
      token: token.asset,
      balance: accounts.map(t => Number(formatBalance(t.tokens[index].assetBalance, token.asset.decimals)) || 0).reduce((sum, x) => sum + x, 0)
    }
  })
}
