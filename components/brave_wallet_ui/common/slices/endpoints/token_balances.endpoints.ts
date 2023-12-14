// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { assert, assertNotReached } from 'chrome://resources/js/assert.js'
import { eachLimit } from 'async'

// constants
import { BraveWallet, CoinTypes } from '../../../constants/types'
import { coinTypesMapping } from '../constants'

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import Amount from '../../../utils/amount'
import {
  GetBlockchainTokenIdArg,
  isNativeAsset
} from '../../../utils/asset-utils'
import { handleEndpointError } from '../../../utils/api-utils'
import { getAccountBalancesKey } from '../../../utils/balance-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { networkSupportsAccount } from '../../../utils/network-utils'
import { cacher } from '../../../utils/query-cache-utils'
import { networkEntityAdapter } from '../entities/network.entity'
import {
  TokenBalancesRegistry,
  TokenBalancesForChainId
} from '../entities/token-balance.entity'

type BalanceNetwork = Pick<
  BraveWallet.NetworkInfo,
  'chainId' | 'coin' | 'supportedKeyrings'
>

type GetHardwareAccountDiscoveryBalanceArg = {
  coin: BraveWallet.CoinType
  chainId: string
  address: string
}

type GetAccountTokenCurrentBalanceArg = {
  accountId: BraveWallet.AccountId
  token: GetBlockchainTokenIdArg
}

type GetSPLTokenBalancesForAddressAndChainIdArg = {
  accountId: BraveWallet.AccountId
  chainId: string

  /**
   * optional, if not provided, will fetch all tokens using
   * getTokenAccountsByOwner.
   */
  tokens?: GetBlockchainTokenIdArg[]
  coin: typeof CoinTypes.SOL
}

type GetTokenBalancesForAddressAndChainIdArg = {
  accountId: BraveWallet.AccountId
  tokens: GetBlockchainTokenIdArg[]
  chainId: string
  coin:
    | typeof CoinTypes.ETH
    | typeof CoinTypes.FIL
    | typeof CoinTypes.BTC
    | typeof CoinTypes.ZEC
}

type GetTokenBalancesForChainIdArg =
  | GetSPLTokenBalancesForAddressAndChainIdArg
  | GetTokenBalancesForAddressAndChainIdArg

type GetTokenBalancesRegistryArg = {
  accountIds: BraveWallet.AccountId[]
  networks: BalanceNetwork[]
  useAnkrBalancesFeature: boolean
}

export const tokenBalancesEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getAccountTokenCurrentBalance: query<
      string,
      GetAccountTokenCurrentBalanceArg
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        const { data: api } = baseQuery(undefined)
        const { jsonRpcService, bitcoinWalletService, zcashWalletService } = api

        try {
          return fetchAccountTokenCurrentBalance({
            arg,
            bitcoinWalletService,
            jsonRpcService,
            zcashWalletService
          })
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get account token current balance',
            error
          )
        }
      },
      providesTags: (result, error, { token }) =>
        cacher.cacheByBlockchainTokenArg('AccountTokenCurrentBalance')(
          result,
          error,
          token
        )
    }),

    getTokenBalancesForChainId: query<
      TokenBalancesRegistry,
      GetTokenBalancesForChainIdArg[]
    >({
      queryFn: async (args, { endpoint }, extraOptions, baseQuery) => {
        const {
          bitcoinWalletService,
          zcashWalletService,
          jsonRpcService,
          braveWalletService
        } = baseQuery(undefined).data

        try {
          const result = await fetchTokenBalanceRegistryForAccountsAndChainIds({
            args,
            bitcoinWalletService,
            braveWalletService,
            jsonRpcService,
            zcashWalletService
          })
          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to fetch getTokenBalancesForChainId(
                ${JSON.stringify(args, undefined, 2)}
              )`,
            error
          )
        }
      },
      providesTags: (result, err, args) =>
        err
          ? ['TokenBalancesForChainId', 'UNKNOWN_ERROR']
          : args.flatMap(({ tokens, accountId, coin, chainId }) =>
              tokens
                ? tokens.map((token) => ({
                    type: 'TokenBalancesForChainId',
                    id: [
                      accountId.uniqueKey,
                      coin,
                      chainId,
                      token.contractAddress
                    ].join('-')
                  }))
                : [
                    {
                      type: 'TokenBalancesForChainId',
                      id: `${accountId.uniqueKey}-${coin}-${chainId}`
                    }
                  ]
            ) || ['TokenBalancesForChainId']
    }),

    getTokenBalancesRegistry: query<
      TokenBalancesRegistry,
      GetTokenBalancesRegistryArg
    >({
      queryFn: async (args, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)

          const {
            braveWalletService,
            jsonRpcService,
            bitcoinWalletService,
            zcashWalletService
          } = api
          const { getUserTokensRegistry } = cache

          const ankrSupportedNetworks: BalanceNetwork[] = []
          const nonAnkrSupportedNetworks: BalanceNetwork[] =
            args.useAnkrBalancesFeature ? [] : args.networks

          if (args.useAnkrBalancesFeature) {
            const { chainIds: ankrSupportedChainIds } =
              await braveWalletService.getAnkrSupportedChainIds()

            for (const network of args.networks) {
              if (ankrSupportedChainIds.includes(network.chainId)) {
                ankrSupportedNetworks.push(network)
              } else {
                nonAnkrSupportedNetworks.push(network)
              }
            }
          }

          const userTokens = await getUserTokensRegistry()

          let tokenBalancesRegistry: TokenBalancesRegistry = {}

          await eachLimit(
            args.accountIds,
            args.accountIds.length,
            async (accountId: BraveWallet.AccountId) => {
              const nonAnkrSupportedAccountNetworks =
                nonAnkrSupportedNetworks.filter((network) =>
                  networkSupportsAccount(network, accountId)
                )

              const nonAnkrBalancesRegistry: TokenBalancesRegistry = {}

              if (nonAnkrSupportedAccountNetworks.length) {
                await eachLimit(
                  nonAnkrSupportedAccountNetworks,
                  3,
                  async (network: BraveWallet.NetworkInfo) => {
                    assert(coinTypesMapping[network.coin] !== undefined)
                    try {
                      const partialRegistry: TokenBalancesRegistry =
                        await fetchTokenBalanceRegistryForAccountsAndChainIds({
                          args:
                            network.coin === CoinTypes.SOL
                              ? [
                                  {
                                    accountId,
                                    coin: CoinTypes.SOL,
                                    chainId: network.chainId
                                  }
                                ]
                              : [
                                  {
                                    accountId,
                                    coin: coinTypesMapping[network.coin],
                                    chainId: network.chainId,
                                    tokens: getEntitiesListFromEntityState(
                                      userTokens,
                                      userTokens.idsByChainId[
                                        networkEntityAdapter.selectId({
                                          coin: network.coin,
                                          chainId: network.chainId
                                        })
                                      ]
                                    )
                                  }
                                ],
                          bitcoinWalletService,
                          braveWalletService,
                          jsonRpcService,
                          zcashWalletService
                        })

                      // add partial registry data to main registry
                      for (const [
                        uniqueKey,
                        balancesByChainId
                      ] of Object.entries(partialRegistry)) {
                        if (
                          !nonAnkrBalancesRegistry.hasOwnProperty(uniqueKey)
                        ) {
                          nonAnkrBalancesRegistry[uniqueKey] = balancesByChainId
                        } else {
                          nonAnkrBalancesRegistry[uniqueKey] = {
                            ...nonAnkrBalancesRegistry[uniqueKey],
                            ...balancesByChainId
                          }
                        }
                      }
                    } catch (error) {
                      console.error(error)
                    }
                  }
                )
              }

              const ankrSupportedAccountChainIds = ankrSupportedNetworks
                .filter((network) => networkSupportsAccount(network, accountId))
                .map(({ chainId }) => chainId)

              const ankrAssetBalances: BraveWallet.AnkrAssetBalance[] =
                ankrSupportedAccountChainIds.length
                  ? await fetchAnkrAccountBalances(
                      jsonRpcService,
                      accountId,
                      ankrSupportedAccountChainIds
                    )
                  : []

              const accountKey = getAccountBalancesKey(accountId)

              const accountBalancesRegistry: TokenBalancesRegistry =
                nonAnkrBalancesRegistry

              for (const { asset, balance } of ankrAssetBalances) {
                accountBalancesRegistry[accountKey] = {
                  ...(accountBalancesRegistry[accountKey] || {}),
                  [asset.chainId]: {
                    ...((accountBalancesRegistry[accountKey] || {})[
                      asset.chainId
                    ] || {}),
                    [asset.contractAddress]: balance
                  }
                }
              }

              tokenBalancesRegistry = {
                ...tokenBalancesRegistry,
                ...accountBalancesRegistry
              }
            }
          )

          return {
            data: tokenBalancesRegistry
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Error calling getTokenBalancesRegistry: ${error}`,
            error
          )
        }
      },
      providesTags: (result, err, args) =>
        err
          ? ['TokenBalances', 'UNKNOWN_ERROR']
          : args.accountIds.flatMap((accountId) => {
              const networkKeys = args.networks
                .filter((network) => accountId.coin === network.coin)
                .map((network) => network.chainId)
              const accountKey = getAccountBalancesKey(accountId)
              return networkKeys.map((networkKey) => ({
                type: 'TokenBalances',
                id: `${accountKey}-${networkKey}`
              }))
            })
    }),

    getHardwareAccountDiscoveryBalance: query<
      string,
      GetHardwareAccountDiscoveryBalanceArg
    >({
      queryFn: async (
        { coin, chainId, address },
        { endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          // apiProxy
          const { jsonRpcService } = baseQuery(undefined).data

          switch (coin) {
            case BraveWallet.CoinType.SOL: {
              const { balance, errorMessage } =
                await jsonRpcService.getSolanaBalance(address, chainId)

              if (errorMessage) {
                console.log(`getBalance error: ${errorMessage}`)
                return {
                  error: errorMessage
                }
              }

              return {
                data: Amount.normalize(balance.toString())
              }
            }

            case BraveWallet.CoinType.FIL:
            case BraveWallet.CoinType.ETH: {
              const { balance, errorMessage } = await jsonRpcService.getBalance(
                address,
                coin,
                chainId
              )

              if (errorMessage) {
                console.log(`getBalance error: ${errorMessage}`)
                return {
                  error: errorMessage
                }
              }

              return {
                data: Amount.normalize(balance)
              }
            }

            default: {
              assertNotReached(`Unknown coin ${coin}`)
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to fetch balance(
                ${JSON.stringify({ coin, chainId, address }, undefined, 2)}
              )`,
            error
          )
        }
      },
      providesTags: (result, err, arg) => [
        {
          type: 'HardwareAccountDiscoveryBalance',
          id: [arg.coin, arg.chainId, arg.address].join('-')
        }
      ]
    })
  }
}

// Internals
async function fetchAnkrAccountBalances(
  jsonRpcService: BraveWallet.JsonRpcServiceRemote,
  accountId: BraveWallet.AccountId,
  chainIds: string[]
) {
  const { balances, error, errorMessage } =
    await jsonRpcService.ankrGetAccountBalances(accountId.address, chainIds)

  if (error !== BraveWallet.ProviderError.kSuccess || errorMessage) {
    console.log(
      `ankrGetAccountBalance error: ${error}
                      msg: ${errorMessage}`
    )
  }

  return balances || []
}

async function fetchAccountCurrentNativeBalance({
  arg: { accountId, token },
  jsonRpcService,
  bitcoinWalletService,
  zcashWalletService
}: {
  arg: GetAccountTokenCurrentBalanceArg
  jsonRpcService: BraveWallet.JsonRpcServiceRemote
  bitcoinWalletService: BraveWallet.BitcoinWalletServiceRemote
  zcashWalletService: BraveWallet.ZCashWalletServiceRemote
}): Promise<
  { data: string; error?: undefined } | { error: string; data?: undefined }
> {
  // LOCALHOST
  if (
    token.chainId === BraveWallet.LOCALHOST_CHAIN_ID &&
    accountId.coin !== BraveWallet.CoinType.SOL
  ) {
    const { balance, error, errorMessage } = await jsonRpcService.getBalance(
      accountId.address,
      accountId.coin,
      token.chainId
    )

    // LOCALHOST will error until a local instance is detected
    // return a '0' balance until it's detected.
    if (error !== 0) {
      console.log(`getBalance error: ${errorMessage}`)
      return {
        data: Amount.zero().format()
      }
    }

    return {
      data: Amount.normalize(balance)
    }
  }

  // NON-LOCALHOST
  switch (accountId.coin) {
    case BraveWallet.CoinType.SOL: {
      const { balance, error } = await jsonRpcService.getSolanaBalance(
        accountId.address,
        token.chainId
      )

      if (token.chainId === BraveWallet.LOCALHOST_CHAIN_ID && error !== 0) {
        return { data: Amount.zero().format() }
      }

      return {
        data: Amount.normalize(balance.toString())
      }
    }

    case BraveWallet.CoinType.FIL:
    case BraveWallet.CoinType.ETH: {
      const { balance, error, errorMessage } = await jsonRpcService.getBalance(
        accountId.address,
        accountId.coin,
        token.chainId
      )

      if (error && errorMessage) {
        console.log(`getBalance error: ${errorMessage}`)
        return {
          error: errorMessage
        }
      }

      return {
        data: Amount.normalize(balance)
      }
    }

    case BraveWallet.CoinType.BTC: {
      const { balance, errorMessage } = await bitcoinWalletService.getBalance(
        accountId
      )

      if (errorMessage || balance === null) {
        console.log(`getBalance error: ${errorMessage}`)
        return {
          error: errorMessage || 'Unknown error'
        }
      }

      return {
        data: Amount.normalize(balance.totalBalance.toString())
      }
    }

    case BraveWallet.CoinType.ZEC: {
      const { balance, errorMessage } = await zcashWalletService.getBalance(
        token.chainId,
        accountId
      )

      if (errorMessage || balance === null) {
        console.log(`getBalance error: ${errorMessage}`)
        return {
          error: errorMessage || 'Unknown error'
        }
      }

      return {
        data: Amount.normalize(balance.totalBalance.toString())
      }
    }

    default: {
      assertNotReached(`Unknown coin ${accountId.coin}`)
    }
  }
}

async function fetchAccountTokenCurrentBalance({
  arg: { accountId, token },
  jsonRpcService,
  bitcoinWalletService,
  zcashWalletService
}: {
  arg: GetAccountTokenCurrentBalanceArg
  jsonRpcService: BraveWallet.JsonRpcServiceRemote
  bitcoinWalletService: BraveWallet.BitcoinWalletServiceRemote
  zcashWalletService: BraveWallet.ZCashWalletServiceRemote
}): Promise<
  { data: string; error?: undefined } | { error: string; data?: undefined }
> {
  // Native asset balances
  if (isNativeAsset(token)) {
    return fetchAccountCurrentNativeBalance({
      arg: { accountId, token },
      jsonRpcService,
      bitcoinWalletService,
      zcashWalletService
    })
  }

  // Token Balances
  switch (accountId.coin) {
    // Ethereum Network tokens
    case BraveWallet.CoinType.ETH: {
      const { balance, error, errorMessage } = token.isErc721
        ? await jsonRpcService.getERC721TokenBalance(
            token.contractAddress,
            token.tokenId ?? '',
            accountId.address,
            token.chainId
          )
        : await jsonRpcService.getERC20TokenBalance(
            token.contractAddress,
            accountId.address,
            token.chainId
          )

      if (error && errorMessage) {
        return { error: errorMessage }
      }

      return {
        data: Amount.normalize(balance)
      }
    }
    // Solana Network Tokens
    case BraveWallet.CoinType.SOL: {
      const { amount, uiAmountString, error, errorMessage } =
        await jsonRpcService.getSPLTokenAccountBalance(
          accountId.address,
          token.contractAddress,
          token.chainId
        )

      if (error && errorMessage) {
        return { error: errorMessage }
      }

      return {
        data: token.isNft ? uiAmountString : amount
      }
    }

    // Other network type tokens
    default: {
      return {
        data: Amount.zero().format()
      }
    }
  }
}

async function fetchAccountTokenBalanceRegistryForChainId({
  arg,
  bitcoinWalletService,
  jsonRpcService,
  zcashWalletService,
  braveWalletService
}: {
  arg: GetTokenBalancesForChainIdArg
  jsonRpcService: BraveWallet.JsonRpcServiceRemote
  bitcoinWalletService: BraveWallet.BitcoinWalletServiceRemote
  zcashWalletService: BraveWallet.ZCashWalletServiceRemote
  braveWalletService: BraveWallet.BraveWalletServiceRemote
}): Promise<TokenBalancesRegistry> {
  const accountBalanceKey = getAccountBalancesKey(arg.accountId)

  // Construct arg to query native token for use in case the
  // optimized balance fetcher kicks in.
  const nativeTokenArg = arg.tokens
    ? arg.tokens.find(isNativeAsset)
    : {
        coin: arg.coin,
        chainId: arg.chainId,
        contractAddress: '',
        isErc721: false,
        isNft: false,
        tokenId: ''
      }

  const nonNativeTokens = (arg.tokens ?? []).filter(
    (token) => !isNativeAsset(token)
  )

  const baseTokenBalances: TokenBalancesForChainId = {}

  if (nativeTokenArg) {
    const { data: balance } = await fetchAccountTokenCurrentBalance({
      arg: {
        accountId: arg.accountId,
        token: nativeTokenArg
      },
      bitcoinWalletService,
      jsonRpcService,
      zcashWalletService
    })

    if (balance) {
      baseTokenBalances[nativeTokenArg.contractAddress] = balance
    }
  }

  if (arg.coin === CoinTypes.ETH) {
    // jsonRpcService.getERC20TokenBalances cannot handle
    // native assets
    if (nonNativeTokens.length === 0) {
      return {
        [accountBalanceKey]: {
          [arg.chainId]: baseTokenBalances
        }
      }
    }

    // TODO(josheleonard): aggresively cache this response
    // since it never changes
    const {
      chainIds: supportedChainIds //
    } = await braveWalletService.getBalanceScannerSupportedChains()

    if (supportedChainIds.includes(arg.chainId)) {
      const result = await jsonRpcService.getERC20TokenBalances(
        nonNativeTokens.map((token) => token.contractAddress),
        arg.accountId.address,
        arg.chainId
      )
      if (result.error === BraveWallet.ProviderError.kSuccess) {
        return {
          [accountBalanceKey]: {
            [arg.chainId]: result.balances.reduce(
              (acc, { balance, contractAddress }) => {
                const token = arg.tokens.find(
                  (token) => token.contractAddress === contractAddress
                )

                const balanceAmount = balance ? new Amount(balance) : undefined

                if (balanceAmount && token) {
                  return {
                    ...acc,
                    [contractAddress]: balanceAmount.format()
                  }
                }

                return acc
              },
              baseTokenBalances
            )
          }
        }
      } else {
        throw new Error(
          'Error calling ' +
            'jsonRpcService.getERC20TokenBalances: ' +
            `error=${result.errorMessage} arg=` +
            JSON.stringify(arg, undefined, 2)
        )
      }
    }
  }

  if (arg.coin === CoinTypes.SOL && !arg.tokens) {
    const result = await jsonRpcService.getSPLTokenBalances(
      arg.accountId.address,
      arg.chainId
    )

    if (result.error === BraveWallet.ProviderError.kSuccess) {
      return {
        [accountBalanceKey]: {
          [arg.chainId]: result.balances.reduce((acc, balanceResult) => {
            if (balanceResult.amount) {
              return {
                ...acc,
                [balanceResult.mint]: Amount.normalize(balanceResult.amount)
              }
            }

            return acc
          }, baseTokenBalances)
        }
      }
    } else {
      throw new Error(
        `Error calling jsonRpcService.getSPLTokenBalances:
              error=${result.errorMessage}
              arg=` + JSON.stringify(arg, undefined, 2)
      )
    }
  }

  // Fallback to fetching individual balances

  const combinedBalancesRegistry: TokenBalancesRegistry = {
    [accountBalanceKey]: {
      [arg.chainId]: baseTokenBalances
    }
  }

  await eachLimit(
    nonNativeTokens,
    10,
    async (token: BraveWallet.BlockchainToken) => {
      const { data: result } = await fetchAccountTokenCurrentBalance({
        arg: {
          accountId: arg.accountId,
          token
        },
        bitcoinWalletService,
        jsonRpcService,
        zcashWalletService
      })

      if (result && new Amount(result).gt(0)) {
        combinedBalancesRegistry[accountBalanceKey][arg.chainId][
          token.contractAddress
        ] = result
      }
    }
  )

  return combinedBalancesRegistry
}

async function fetchTokenBalanceRegistryForAccountsAndChainIds({
  args,
  jsonRpcService,
  bitcoinWalletService,
  braveWalletService,
  zcashWalletService
}: {
  args: GetTokenBalancesForChainIdArg[]
  jsonRpcService: BraveWallet.JsonRpcServiceRemote
  bitcoinWalletService: BraveWallet.BitcoinWalletServiceRemote
  zcashWalletService: BraveWallet.ZCashWalletServiceRemote
  braveWalletService: BraveWallet.BraveWalletServiceRemote
}): Promise<TokenBalancesRegistry> {
  let tokenBalancesRegistry = {}

  await eachLimit(args, 1, async (arg: GetTokenBalancesForChainIdArg) => {
    const partialRegistry = await fetchAccountTokenBalanceRegistryForChainId({
      arg,
      bitcoinWalletService,
      braveWalletService,
      jsonRpcService,
      zcashWalletService
    })

    const [[uniqueKey, balancesByChainId]] = Object.entries(partialRegistry)

    if (!tokenBalancesRegistry.hasOwnProperty(uniqueKey)) {
      tokenBalancesRegistry[uniqueKey] = balancesByChainId
    } else {
      tokenBalancesRegistry[uniqueKey] = {
        ...tokenBalancesRegistry[uniqueKey],
        ...balancesByChainId
      }
    }
  })

  return tokenBalancesRegistry
}
