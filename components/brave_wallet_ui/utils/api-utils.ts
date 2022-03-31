import { BraveWallet, WalletAccountType, GetFlattenedAccountBalancesReturnInfo } from '../constants/types'

export const getTokenParam = (token: BraveWallet.BlockchainToken): string => {
  if (token.coingeckoId) {
    return token.coingeckoId
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID

  if (!isEthereumNetwork) {
    return token.symbol.toLowerCase()
  }

  if (token.contractAddress === '') {
    return token.symbol.toLowerCase()
  }

  return token.contractAddress
}

// This will get the sum balance for each token between all accounts
export const getFlattenedAccountBalances = (accounts: WalletAccountType[], userVisibleTokensInfo: BraveWallet.BlockchainToken[]): GetFlattenedAccountBalancesReturnInfo[] => {
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
            : account.nativeBalanceRegistry[token.chainId]

          return balance || '0'
        })
        .reduce((a, b) => a + Number(b), 0)
    }
  })
}
