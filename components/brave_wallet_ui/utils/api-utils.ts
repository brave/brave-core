import { BraveWallet, WalletAccountType, GetFlattenedAccountBalancesReturnInfo } from '../constants/types'
import { ETH } from '../options/asset-options'

export const GetTokenParam = (selectedNetwork: BraveWallet.EthereumChain, token: BraveWallet.BlockchainToken): string => {
  if (token.coingeckoId) {
    return token.coingeckoId
  }

  const isEthereumNetwork = selectedNetwork.chainId === BraveWallet.MAINNET_CHAIN_ID

  if (!isEthereumNetwork) {
    return token.symbol.toLowerCase()
  }

  return token.symbol.toLowerCase() === ETH.symbol.toLowerCase()
    ? token.symbol.toLowerCase()
    : token.contractAddress
}

// This will get the sum balance for each token between all accounts
export const GetFlattenedAccountBalances = (accounts: WalletAccountType[], userVisibleTokensInfo: BraveWallet.BlockchainToken[]): GetFlattenedAccountBalancesReturnInfo[] => {
  if (accounts.length === 0) {
    return []
  }

  return userVisibleTokensInfo.map((token) => {
    return {
      token: token,
      balance: accounts
        .map(account => {
          const balance = token.contractAddress
            ? account.tokenBalanceRegistry[token.contractAddress.toLowerCase()]
            : account.balance

          return balance || '0'
        })
        .reduce((a, b) => a + Number(b), 0)
    }
  })
}
