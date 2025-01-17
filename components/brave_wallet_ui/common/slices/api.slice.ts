// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { BraveWallet } from '../../constants/types'

// entities
import {
  networkEntityAdapter,
  selectMainnetNetworksFromQueryResult,
  selectAllNetworksFromQueryResult,
  selectOffRampNetworksFromQueryResult,
  selectOnRampNetworksFromQueryResult,
  selectVisibleNetworksFromQueryResult
} from './entities/network.entity'

// api
import { createWalletApiBase } from './api-base.slice'
import { transactionSimulationEndpoints } from './endpoints/tx-simulation.endpoints'
import { braveRewardsApiEndpoints } from './endpoints/rewards.endpoints'
import { p3aEndpoints } from './endpoints/p3a.endpoints'
import { pricingEndpoints } from './endpoints/pricing.endpoints'
import { nftsEndpoints } from './endpoints/nfts.endpoints'
import { qrCodeEndpoints } from './endpoints/qr-code.endpoints'

// utils
import { handleEndpointError } from '../../utils/api-utils'
import { walletEndpoints } from './endpoints/wallet.endpoints'
import { tokenEndpoints } from './endpoints/token.endpoints'
import { onRampEndpoints } from './endpoints/on-ramp.endpoints'
import { offRampEndpoints } from './endpoints/off-ramp.endpoints'
import { coingeckoEndpoints } from './endpoints/coingecko-endpoints'
import {
  tokenSuggestionsEndpoints //
} from './endpoints/token_suggestions.endpoints'
import { addressEndpoints } from './endpoints/address.endpoints'
import { accountEndpoints } from './endpoints/account.endpoints'
import { networkEndpoints } from './endpoints/network.endpoints'
import { coinMarketEndpoints } from './endpoints/market.endpoints'
import { tokenBalancesEndpoints } from './endpoints/token_balances.endpoints'
import { fiatCurrencyEndpoints } from './endpoints/fiat_currency.endpoints'
import { sitePermissionEndpoints } from './endpoints/site_permissions.endpoints'
import { transactionEndpoints } from './endpoints/transaction.endpoints'
import { swapEndpoints } from './endpoints/swap.endpoints'
import { encryptionEndpoints } from './endpoints/encryption.endpoints'
import { signingEndpoints } from './endpoints/signing.endpoints'
import { dappRadarEndpoints } from './endpoints/dapp_radar.endpoints'
import { meldIntegrationEndpoints } from './endpoints/meld_integration.endpoints'
import { zcashEndpoints } from './endpoints/zcash.endpoints'

export function createWalletApi() {
  // base to add endpoints to
  return (
    createWalletApiBase()
      .injectEndpoints({
        endpoints: ({ mutation, query }) => {
          return {
            getAddressByteCode: query<
              string,
              { address: string; coin: number; chainId: string }
            >({
              queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
                try {
                  const { jsonRpcService } = baseQuery(undefined).data
                  const { bytecode, error, errorMessage } =
                    await jsonRpcService.getCode(
                      arg.address,
                      arg.coin,
                      arg.chainId
                    )
                  if (error !== 0 && errorMessage) {
                    return {
                      error: errorMessage
                    }
                  }
                  return {
                    data: bytecode
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to fetch bytecode for address: ${arg.address}.`,
                    error
                  )
                }
              }
            })
          }
        }
      })
      // panel endpoints
      .injectEndpoints({
        endpoints: ({ mutation, query }) => ({
          openPanelUI: mutation<boolean, void>({
            queryFn(arg, api, extraOptions, baseQuery) {
              const { panelHandler } = baseQuery(undefined).data
              panelHandler?.showUI()
              return { data: true }
            }
          }),
          closePanelUI: mutation<boolean, void>({
            queryFn(arg, api, extraOptions, baseQuery) {
              const { panelHandler } = baseQuery(undefined).data
              panelHandler?.closeUI()
              return { data: true }
            }
          })
        })
      })
      // Wallet management endpoints
      .injectEndpoints({ endpoints: walletEndpoints })
      // Token balance endpoints
      .injectEndpoints({ endpoints: tokenBalancesEndpoints })
      // brave rewards endpoints
      .injectEndpoints({ endpoints: braveRewardsApiEndpoints })
      // tx simulation
      .injectEndpoints({ endpoints: transactionSimulationEndpoints })
      // p3a endpoints
      .injectEndpoints({ endpoints: p3aEndpoints })
      // price history endpoints
      .injectEndpoints({ endpoints: pricingEndpoints })
      // nfts endpoints
      .injectEndpoints({ endpoints: nftsEndpoints })
      // onRamp endpoints
      .injectEndpoints({ endpoints: onRampEndpoints })
      // offRamp endpoints
      .injectEndpoints({ endpoints: offRampEndpoints })
      // coingecko endpoints
      .injectEndpoints({ endpoints: coingeckoEndpoints })
      // token suggestion request endpoints
      .injectEndpoints({ endpoints: tokenSuggestionsEndpoints })
      // QR Code generator endpoints
      .injectEndpoints({ endpoints: qrCodeEndpoints })
      // ENS, SNS, UD Address endpoints
      .injectEndpoints({ endpoints: addressEndpoints })
      // Account management endpoints
      .injectEndpoints({ endpoints: accountEndpoints })
      // Blockchain Network management endpoints
      .injectEndpoints({ endpoints: networkEndpoints })
      // Blockchain Token (User assets) management endpoints
      .injectEndpoints({ endpoints: tokenEndpoints })
      // Transaction endpoints
      .injectEndpoints({ endpoints: transactionEndpoints })
      // Coin market endpoints
      .injectEndpoints({ endpoints: coinMarketEndpoints })
      // Fiat currency endpoints
      .injectEndpoints({ endpoints: fiatCurrencyEndpoints })
      // Site permission (connected accounts) endpoints
      .injectEndpoints({ endpoints: sitePermissionEndpoints })
      // Brave Swap endpoints
      .injectEndpoints({ endpoints: swapEndpoints })
      // Encryption endpoints
      .injectEndpoints({ endpoints: encryptionEndpoints })
      // Message Signing endpoints
      .injectEndpoints({ endpoints: signingEndpoints })
      // dApp Radar Endpoints
      .injectEndpoints({ endpoints: dappRadarEndpoints })
      // meld integration endpoints
      .injectEndpoints({ endpoints: meldIntegrationEndpoints })
      // zcash endpoints
      .injectEndpoints({ endpoints: zcashEndpoints })
  )
}

export type WalletApi = ReturnType<typeof createWalletApi>
export const walletApi: WalletApi = createWalletApi()

export const {
  middleware: walletApiMiddleware,
  reducer: walletApiReducer,
  reducerPath: walletApiReducerPath,
  // hooks
  useAcknowledgePendingAddChainRequestMutation,
  useAcknowledgeSwitchChainRequestMutation,
  useAddAccountMutation,
  useAddUserTokenMutation,
  useApproveERC20AllowanceMutation,
  useApproveHardwareTransactionMutation,
  useApproveOrDeclineTokenSuggestionMutation,
  useApproveTransactionMutation,
  useCancelConnectToSiteMutation,
  useCancelTransactionMutation,
  useCheckExternalWalletPasswordMutation,
  useClosePanelUIMutation,
  useCompleteWalletBackupMutation,
  useConnectToSiteMutation,
  useCreateWalletMutation,
  useDiscoverAssetsMutation,
  useEnableEnsOffchainLookupMutation,
  useGenerateReceiveAddressMutation,
  useGenerateSwapQuoteMutation,
  useGenerateSwapTransactionMutation,
  useGetAccountInfosRegistryQuery,
  useGetAccountTokenCurrentBalanceQuery,
  useGetActiveOriginConnectedAccountIdsQuery,
  useGetActiveOriginQuery,
  useGetAddressByteCodeQuery,
  useGetAddressFromNameServiceUrlQuery,
  useGetAllKnownNetworksQuery,
  useGetBitcoinBalancesQuery,
  useGetBuyUrlQuery,
  useGetChainTipStatusQuery,
  useGetCoingeckoIdQuery,
  useGetCoinMarketQuery,
  useGetDefaultEthereumWalletQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetDefaultSolanaWalletQuery,
  useGetERC20AllowanceQuery,
  useGetEthAddressChecksumQuery,
  useGetEVMTransactionSimulationQuery,
  useGetFVMAddressQuery,
  useGetGasEstimation1559Query,
  useGetHardwareAccountDiscoveryBalanceQuery,
  useGetHasTransactionSimulationSupportQuery,
  useGetIpfsGatewayTranslatedNftUrlQuery,
  useGetIsBase58EncodedSolPubkeyQuery,
  useGetIsMetaMaskInstalledQuery,
  useGetIsPrivateWindowQuery,
  useGetIsShieldingAvailableQuery,
  useGetIsTokenOwnedByUserQuery,
  useGetIsTxSimulationOptInStatusQuery,
  useGetIsWalletBackedUpQuery,
  useGetNetworksRegistryQuery,
  useGetNftAssetIdsByCollectionRegistryQuery,
  useGetNftDiscoveryEnabledStatusQuery,
  useGetNftMetadataQuery,
  useGetNftOwnerQuery,
  useGetOffRampAssetsQuery,
  useGetOnRampAssetsQuery,
  useGetOnRampFiatCurrenciesQuery,
  useGetPendingAddChainRequestQuery,
  useGetPendingDecryptRequestQuery,
  useGetPendingGetEncryptionPublicKeyRequestQuery,
  useGetPendingSignSolTransactionsRequestsQuery,
  useGetPendingSignMessageErrorsQuery,
  useGetPendingSignMessageRequestsQuery,
  useGetPendingSwitchChainRequestQuery,
  useGetPendingTokenSuggestionRequestsQuery,
  useGetPriceHistoryQuery,
  useGetPricesHistoryQuery,
  useGetQrCodeImageQuery,
  useGetRewardsInfoQuery,
  useGetSelectedAccountIdQuery,
  useGetSelectedChainQuery,
  useGetSimpleHashSpamNftsQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetSolanaTransactionSimulationQuery,
  useGetSolanaSignTransactionsRequestSimulationQuery,
  useGetSwapSupportedNetworksQuery,
  useGetTokenBalancesForChainIdQuery,
  useGetTokenBalancesRegistryQuery,
  useGetTokenInfoQuery,
  useGetTokenSpotPricesQuery,
  useGetTokensRegistryQuery,
  useGetTopDappsQuery,
  useGetTransactionQuery,
  useGetTransactionsQuery,
  useGetUserTokensRegistryQuery,
  useGetWalletsToImportQuery,
  useGetZCashAccountInfoQuery,
  useHideNetworksMutation,
  useImportAccountFromJsonMutation,
  useImportAccountMutation,
  useImportBtcAccountMutation,
  useImportFilAccountMutation,
  useImportFromCryptoWalletsMutation,
  useImportFromMetaMaskMutation,
  useImportHardwareAccountsMutation,
  useInvalidateAccountInfosMutation,
  useInvalidateSelectedAccountMutation,
  useInvalidateTransactionsCacheMutation,
  useLazyGetAccountInfosRegistryQuery,
  useLazyGetAccountTokenCurrentBalanceQuery,
  useLazyGetAddressByteCodeQuery,
  useLazyGetAllKnownNetworksQuery,
  useLazyGetBitcoinBalancesQuery,
  useLazyGetBuyUrlQuery,
  useLazyGetChainTipStatusQuery,
  useLazyGetDefaultFiatCurrencyQuery,
  useLazyGetERC20AllowanceQuery,
  useLazyGetEVMTransactionSimulationQuery,
  useLazyGetGasEstimation1559Query,
  useLazyGetIsTxSimulationOptInStatusQuery,
  useLazyGetIsShieldingAvailableQuery,
  useLazyGetNetworksRegistryQuery,
  useLazyGetNftDiscoveryEnabledStatusQuery,
  useLazyGetPendingTokenSuggestionRequestsQuery,
  useLazyGetSelectedAccountIdQuery,
  useLazyGetSelectedChainQuery,
  useLazyGetSellAssetUrlQuery,
  useLazyGetSolanaEstimatedFeeQuery,
  useLazyGetSolanaTransactionSimulationQuery,
  useLazyGetSwapSupportedNetworksQuery,
  useLazyGetTokenBalancesForChainIdQuery,
  useLazyGetTokenBalancesRegistryQuery,
  useLazyGetTokenSpotPricesQuery,
  useLazyGetTokensRegistryQuery,
  useLazyGetTransactionsQuery,
  useLazyGetUserTokensRegistryQuery,
  useLazyGetZCashAccountInfoQuery,
  useLockWalletMutation,
  useMakeAccountShieldedMutation,
  useNewUnapprovedTxAddedMutation,
  useOpenPanelUIMutation,
  usePrefetch,
  useProcessPendingDecryptRequestMutation,
  useProcessPendingGetEncryptionPublicKeyRequestMutation,
  useProcessSignSolTransactionsRequestHardwareMutation,
  useProcessSignSolTransactionsRequestMutation,
  useProcessSignMessageErrorMutation,
  useProcessSignMessageRequestMutation,
  useRefreshNetworkInfoMutation,
  useRejectTransactionsMutation,
  useRemoveAccountMutation,
  useRemoveSitePermissionMutation,
  useRemoveUserTokenMutation,
  useReportOnboardingActionMutation,
  useRequestSitePermissionMutation,
  useRestoreNetworksMutation,
  useRestoreWalletMutation,
  useRetryTransactionMutation,
  useSendBtcTransactionMutation,
  useSendERC20TransferMutation,
  useSendERC721TransferFromMutation,
  useSendETHFilForwarderTransferMutation,
  useSendEvmTransactionMutation,
  useSendFilTransactionMutation,
  useSendSolanaSerializedTransactionMutation,
  useSendSolTransactionMutation,
  useSendSPLTransferMutation,
  useSendZecTransactionMutation,
  useSetAutoLockMinutesMutation,
  useSetDefaultFiatCurrencyMutation,
  useSetIsTxSimulationOptInStatusMutation,
  useSetNetworkMutation,
  useSetNftDiscoveryEnabledMutation,
  useSetSelectedAccountMutation,
  useShowRecoveryPhraseMutation,
  useSignMessageHardwareMutation,
  useSpeedupTransactionMutation,
  useStartShieldSyncMutation,
  useStopShieldSyncMutation,
  useTransactionStatusChangedMutation,
  useUnapprovedTxUpdatedMutation,
  useUnlockWalletMutation,
  useUpdateAccountNameMutation,
  useUpdateNftSpamStatusMutation,
  useUpdateUnapprovedTransactionGasFieldsMutation,
  useUpdateUnapprovedTransactionNonceMutation,
  useUpdateUnapprovedTransactionSpendAllowanceMutation,
  useUpdateUserAssetVisibleMutation,
  useUpdateUserTokenMutation,
  useValidateUnifiedAddressQuery,
  useGetMeldFiatCurrenciesQuery,
  useGetMeldCryptoCurrenciesQuery,
  useGetMeldCountriesQuery,
  useGetMeldServiceProvidersQuery,
  useGetMeldPaymentMethodsQuery,
  useGenerateMeldCryptoQuotesMutation,
  useCreateMeldBuyWidgetMutation
} = walletApi

// Derived Data Queries

export const useGetMainnetsQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectMainnetNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetNetworksQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectAllNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetOffRampNetworksQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectOffRampNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetOnRampNetworksQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectOnRampNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetVisibleNetworksQuery = (
  arg?: undefined | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(arg, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectVisibleNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetNetworkQuery = (
  args:
    | {
        chainId: string
        coin: BraveWallet.CoinType
      }
    | typeof skipToken
) => {
  return useGetNetworksRegistryQuery(
    args === skipToken ? skipToken : undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading || res.isFetching,
        error: res.error,
        data:
          res.data && args !== skipToken
            ? res.data.entities[networkEntityAdapter.selectId(args)]
            : undefined
      })
    }
  )
}

export type WalletApiSliceState = ReturnType<(typeof walletApi)['reducer']>
export type WalletApiSliceStateFromRoot = { walletApi: WalletApiSliceState }
