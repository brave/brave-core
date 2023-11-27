// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { assert, assertNotReached } from 'chrome://resources/js/assert.js'
import { mapLimit } from 'async'

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
  networks: Array<
    Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin' | 'supportedKeyrings'>
  >
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
      queryFn: async (arg, { dispatch, endpoint }, extraOptions, baseQuery) => {
        try {
          const { jsonRpcService, bitcoinWalletService, zcashWalletService } =
            baseQuery(undefined).data // apiProxy
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
      queryFn: async (
        args,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        const {
          bitcoinWalletService,
          zcashWalletService,
          jsonRpcService,
          braveWalletService
        } = baseQuery(undefined).data

        try {
          const result = await fetchTokenBalancesForChainId({
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
      queryFn: async (
        args,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            data: {
              braveWalletService,
              jsonRpcService,
              bitcoinWalletService,
              zcashWalletService
            },
            cache
          } = baseQuery(undefined) // apiProxy
          const {
            getUserTokensRegistry
            // getTokenBalancesForChainId
          } = cache

          const { chainIds: ankrSupportedChainIdsResult } =
            await braveWalletService.getAnkrSupportedChainIds()
          const ankrSupportedChainIds = args.useAnkrBalancesFeature
            ? ankrSupportedChainIdsResult
            : []

          const ankrSupportedNetworks = args.networks.filter((network) =>
            ankrSupportedChainIds.includes(network.chainId)
          )
          const nonAnkrSupportedNetworks = args.networks.filter(
            (network) => !ankrSupportedChainIds.includes(network.chainId)
          )

          const userTokens = await getUserTokensRegistry()

          const tokenBalancesRegistryArray = await mapLimit(
            args.accountIds,
            args.accountIds.length,
            async (accountId: BraveWallet.AccountId) => {
              const ankrSupportedAccountNetworks = ankrSupportedNetworks.filter(
                (network) => networkSupportsAccount(network, accountId)
              )
              const nonAnkrSupportedAccountNetworks =
                nonAnkrSupportedNetworks.filter((network) =>
                  networkSupportsAccount(network, accountId)
                )

              let ankrAssetBalances: BraveWallet.AnkrAssetBalance[] = []

              if (ankrSupportedAccountNetworks.length) {
                const { balances, error, errorMessage } =
                  await jsonRpcService.ankrGetAccountBalances(
                    accountId.address,
                    ankrSupportedAccountNetworks.map(
                      (network) => network.chainId
                    )
                  )

                if (
                  error !== BraveWallet.ProviderError.kSuccess ||
                  errorMessage
                ) {
                  console.log(
                    `ankrGetAccountBalance error: ${error}
                      msg: ${errorMessage}`
                  )
                }

                ankrAssetBalances = balances || []
              }

              let nonAnkrBalancesRegistry: TokenBalancesRegistry = {}

              if (nonAnkrSupportedAccountNetworks.length) {
                const nonAnkrBalancesRegistryArray = await mapLimit(
                  nonAnkrSupportedAccountNetworks,
                  3,
                  async (network: BraveWallet.NetworkInfo) => {
                    assert(coinTypesMapping[network.coin] !== undefined)
                    try {
                      const partialRegistry: TokenBalancesRegistry =
                        await fetchTokenBalancesForChainId({
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

                      return partialRegistry
                    } catch (error) {
                      console.error(error)
                      return {}
                    }
                  }
                )

                nonAnkrBalancesRegistry = nonAnkrBalancesRegistryArray.reduce(
                  (acc, curr) => {
                    for (const [uniqueKey, chainIds] of Object.entries(curr)) {
                      if (!acc.hasOwnProperty(uniqueKey)) {
                        acc[uniqueKey] = chainIds
                      } else {
                        acc[uniqueKey] = {
                          ...acc[uniqueKey],
                          ...chainIds
                        }
                      }
                    }
                    return acc
                  },
                  {}
                )
              }

              return ankrAssetBalances.reduce((acc, { asset, balance }) => {
                const accountKey = getAccountBalancesKey(accountId)

                acc[accountKey] = {
                  ...(acc[accountKey] || {}),
                  [asset.chainId]: {
                    ...((acc[accountKey] || {})[asset.chainId] || {}),
                    [asset.contractAddress]: balance
                  }
                }

                return acc
              }, nonAnkrBalancesRegistry)
            }
          )

          return {
            data: tokenBalancesRegistryArray.reduce(
              (acc, curr) => ({ ...acc, ...curr }),
              {}
            )
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
        const { balance, error, errorMessage } =
          await jsonRpcService.getBalance(
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

async function fetchTokenBalancesForChainId({
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
  const result = await mapLimit(
    args,
    1,
    async (arg: GetTokenBalancesForChainIdArg) => {
      // Construct arg to query native token for use in case the
      // optimised balance fetcher kicks in.
      const nativeTokenArg =
        arg.coin === CoinTypes.ETH
          ? arg.tokens.find(isNativeAsset)
          : arg.tokens // arg.coin is SOL
          ? arg.tokens.find(isNativeAsset)
          : {
              coin: arg.coin,
              chainId: arg.chainId,
              contractAddress: '',
              isErc721: false,
              isNft: false,
              tokenId: ''
            }

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
        const contracts = arg.tokens
          .filter((token) => !isNativeAsset(token))
          .map((token) => token.contractAddress)
        if (contracts.length === 0) {
          return {
            [getAccountBalancesKey(arg.accountId)]: {
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
            contracts,
            arg.accountId.address,
            arg.chainId
          )
          if (result.error === BraveWallet.ProviderError.kSuccess) {
            return {
              [getAccountBalancesKey(arg.accountId)]: {
                [arg.chainId]: result.balances.reduce(
                  (acc, { balance, contractAddress }) => {
                    const token = arg.tokens.find(
                      (token) => token.contractAddress === contractAddress
                    )

                    const balanceAmount = balance
                      ? new Amount(balance)
                      : undefined

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
            [getAccountBalancesKey(arg.accountId)]: {
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
      const tokens = (arg.tokens ?? []).filter((token) => !isNativeAsset(token))

      const combinedBalancesResult = await mapLimit(
        tokens,
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

          return {
            key: token.contractAddress,
            value: result
          }
        }
      )

      return {
        [getAccountBalancesKey(arg.accountId)]: {
          [arg.chainId]: combinedBalancesResult
            .filter(
              (
                item
              ): item is {
                key: string
                value: string
              } => new Amount(item.value || '').gt(0)
            )
            .reduce((obj, item) => {
              obj[item.key] = item.value
              return obj
            }, baseTokenBalances)
        }
      }
    }
  )

  return result.reduce((acc: TokenBalancesRegistry, curr) => {
    const [[uniqueKey, chainIdBalances]] = Object.entries(curr)

    if (!acc.hasOwnProperty(uniqueKey)) {
      acc[uniqueKey] = chainIdBalances
    } else {
      acc[uniqueKey] = {
        ...acc[uniqueKey],
        ...chainIdBalances
      }
    }

    return acc
  }, {})
}
