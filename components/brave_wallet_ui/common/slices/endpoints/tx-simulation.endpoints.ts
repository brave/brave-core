// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  CoinTypes,
  SerializableTransactionInfo
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { toMojoUnion } from '../../../utils/mojo-utils'
import { handleEndpointError } from '../../../utils/api-utils'

export const transactionSimulationEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getIsTxSimulationOptInStatus: query<BraveWallet.BlowfishOptInStatus, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        const { data: api, cache } = baseQuery(undefined)

        const { isTransactionSimulationsFeatureEnabled } =
          await cache.getWalletInfo()

        if (!isTransactionSimulationsFeatureEnabled) {
          return {
            data: BraveWallet.BlowfishOptInStatus.kDenied
          }
        }

        const { status } =
          await api.braveWalletService.getTransactionSimulationOptInStatus()

        return {
          data: status
        }
      },
      providesTags: ['TransactionSimulationsOptIn']
    }),

    setIsTxSimulationOptInStatus: mutation<
      BraveWallet.BlowfishOptInStatus,
      BraveWallet.BlowfishOptInStatus
    >({
      queryFn: async (status, { endpoint }, extraOptions, baseQuery) => {
        const { data: api, cache } = baseQuery(undefined)
        api.braveWalletService.setTransactionSimulationOptInStatus(status)
        cache.clearWalletInfo()
        return {
          data: status
        }
      },
      invalidatesTags: ['TransactionSimulationsOptIn']
    }),

    getHasTransactionSimulationSupport: query<
      boolean,
      { chainId: string; coinType: BraveWallet.CoinType }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { result } =
            await api.simulationService.hasTransactionScanSupport(
              arg.chainId,
              arg.coinType
            )
          return {
            data: result
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to check if this network (${
              arg.chainId //
            }) has transaction simulation support`,
            error
          )
        }
      }
    }),

    getEVMTransactionSimulation: query<
      BraveWallet.EVMSimulationResponse,
      Pick<SerializableTransactionInfo, 'chainId' | 'id'> & {
        coinType: BraveWallet.CoinType
      }
    >({
      queryFn: async (txArg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { simulationService, txService } = api

          const { isTransactionSimulationsFeatureEnabled } =
            await cache.getWalletInfo()

          if (!isTransactionSimulationsFeatureEnabled) {
            throw new Error('Transaction simulation feature is not enabled')
          }

          const { status } =
            await api.braveWalletService.getTransactionSimulationOptInStatus()

          if (status !== BraveWallet.BlowfishOptInStatus.kAllowed) {
            throw new Error('Transaction simulation is not allowed')
          }

          if (txArg.coinType !== BraveWallet.CoinType.ETH) {
            throw new Error(
              `Invalid EVM transaction argument cointype for EVM simulation: ${
                txArg.coinType //
              }`
            )
          }

          const { result } = await simulationService.hasTransactionScanSupport(
            txArg.chainId,
            txArg.coinType
          )

          if (!result) {
            throw new Error(
              `EVM transaction simulation not supported for chain/coin: ${
                txArg.chainId //
              }/${txArg.coinType}`
            )
          }

          const { transactionInfo: tx } = await txService.getTransactionInfo(
            txArg.coinType,
            txArg.chainId,
            txArg.id
          )

          if (!tx) {
            throw new Error(
              `could not lookup EVM tx by meta-ID/coin: ${
                txArg.id //
              }/${txArg.coinType}`
            )
          }

          const { errorResponse, errorString, response } =
            await simulationService.scanEVMTransaction(tx, navigator.language)

          if (errorResponse || errorString) {
            throw new Error(
              `EVM Transaction Simulation error for tx: ${JSON.stringify(
                tx,
                undefined,
                2
              )} -- ${
                errorString //
              } -- ${errorResponse}`
            )
          }

          if (!response) {
            throw new Error('empty EVM simulation response')
          }

          return {
            data: response
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to simulate EVM transaction',
            error
          )
        }
      }
    }),

    getSolanaTransactionSimulation: query<
      BraveWallet.SolanaSimulationResponse,
      {
        mode: keyof Parameters<
          BraveWallet.SimulationServiceRemote['scanSolanaTransaction']
        >[0]
        chainId: string
        id: Pick<
          | BraveWallet.SignAllTransactionsRequest
          | BraveWallet.SignTransactionRequest
          | BraveWallet.TransactionInfo,
          'id'
        >['id']
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { simulationService, braveWalletService, txService } = api

          const { isTransactionSimulationsFeatureEnabled } =
            await cache.getWalletInfo()

          if (!isTransactionSimulationsFeatureEnabled) {
            throw new Error('Transaction simulation feature is not enabled')
          }

          const { status } =
            await api.braveWalletService.getTransactionSimulationOptInStatus()

          if (status !== BraveWallet.BlowfishOptInStatus.kAllowed) {
            throw new Error('Transaction simulation is not allowed')
          }

          const params: Parameters<
            BraveWallet.SimulationServiceRemote['scanSolanaTransaction']
          >[0] = {
            signAllTransactionsRequest: undefined,
            signTransactionRequest: undefined,
            transactionInfo: undefined
          }

          switch (arg.mode) {
            case 'signAllTransactionsRequest': {
              const { requests } = await braveWalletService //
                .getPendingSignAllTransactionsRequests()

              params.signAllTransactionsRequest = requests.find(
                (r) => r.id === arg?.id
              )
              break
            }

            case 'signTransactionRequest': {
              const { requests } =
                await braveWalletService.getPendingSignTransactionRequests()
              params.signTransactionRequest = requests.find(
                (r) => r.id === arg?.id
              )
              break
            }

            case 'transactionInfo': {
              const { transactionInfo } = await txService.getTransactionInfo(
                CoinTypes.SOL,
                arg.chainId,
                arg.id.toString()
              )
              params.transactionInfo = transactionInfo || undefined
              break
            }

            default: {
              throw new Error(`Unsupported SVM simulation mode: ${arg.mode}`)
            }
          }

          const request = params[arg.mode]
          if (!request) {
            throw new Error(
              `Unable to find ${arg.mode || 'MODE'} with Id: ${arg.id}`
            )
          }

          const { result } = await simulationService.hasTransactionScanSupport(
            request.chainId,
            CoinTypes.SOL
          )

          if (!result) {
            throw new Error(
              `Solana transaction simulation not supported for chain/coin: ${
                request.chainId //
              }/${CoinTypes.SOL}`
            )
          }

          const { errorResponse, errorString, response } =
            await simulationService.scanSolanaTransaction(
              toMojoUnion(params, arg.mode),
              navigator.language
            )

          if (errorResponse || errorString) {
            throw new Error(
              `scanSolanaTransaction({${
                //
                JSON.stringify(toMojoUnion(params, arg.mode))
              }}) failed -- ${errorString}: ${errorResponse}`
            )
          }

          if (!response) {
            throw new Error('empty Solana simulation response')
          }

          return {
            data: response
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to simulate Solana transaction',
            error
          )
        }
      }
    })
  }
}
