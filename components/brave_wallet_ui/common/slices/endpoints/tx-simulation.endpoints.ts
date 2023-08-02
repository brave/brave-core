// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  CoinType,
  CoinTypes,
  SerializableTransactionInfo
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { toMojoUnion } from '../../../utils/mojo-utils'

/**
 * disabled until we have an opt-in screen
 */
let isTxSimulationEnabled = false

export const getIsTxSimulationEnabled = () => isTxSimulationEnabled

export const setIsTxSimulationEnabled = (enabled: boolean) => {
  isTxSimulationEnabled = enabled
}

export const transactionSimulationEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getIsTxSimulationEnabled: query<boolean, void>({
      queryFn: async (arg, api, extraOptions, baseQuery) => {
        return {
          data: getIsTxSimulationEnabled()
        }
      },
      providesTags: ['TransactionSimulationsEnabled']
    }),

    setIsTxSimulationEnabled: mutation<boolean, boolean>({
      queryFn: async (enabled, api, extraOptions, baseQuery) => {
        setIsTxSimulationEnabled(enabled)
        return {
          data: enabled
        }
      },
      invalidatesTags: ['TransactionSimulationsEnabled']
    }),

    getEVMTransactionSimulation: query<
      BraveWallet.EVMSimulationResponse,
      Pick<SerializableTransactionInfo, 'chainId' | 'id'> & {
        coinType: CoinType
      }
    >({
      queryFn: async (txArg, { dispatch }, extraOptions, baseQuery) => {
        try {
          if (!getIsTxSimulationEnabled()) {
            return {
              error: 'Transaction simulation is not enabled'
            }
          }

          const api = baseQuery(undefined).data
          const { simulationService, txService } = api

          if (txArg.coinType !== CoinType.ETH) {
            throw new Error(
              `Invalid transaction argument cointype for EVM simulation: ${
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
              `transaction simulation not supported for chain/coin: ${
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
              `could not lookup tx by meta-ID/coin: ${
                txArg.id //
              }/${txArg.coinType}`
            )
          }

          const { errorResponse, errorString, response } =
            await simulationService.scanEVMTransaction(tx, navigator.language)

          if (errorResponse) {
            const error = `${errorString}: ${errorResponse}`
            const message = `Transaction Simulation error: ${error}`
            console.log(message)
            return {
              error
            }
          }

          if (!response) {
            throw new Error('empty simulation response')
          }

          return {
            data: response
          }
        } catch (error) {
          const message = `Failed to simulate transaction: ${error}`
          console.log(message)
          return {
            error: message
          }
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
      queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
        try {
          if (!getIsTxSimulationEnabled()) {
            return {
              error: 'Transaction simulation is not enabled'
            }
          }

          const api = baseQuery(undefined).data
          const { simulationService, braveWalletService, txService } = api
          const { getPendingSignAllTransactionsRequests } = braveWalletService

          const params: Parameters<
            BraveWallet.SimulationServiceRemote['scanSolanaTransaction']
          >[0] = {
            signAllTransactionsRequest: undefined,
            signTransactionRequest: undefined,
            transactionInfo: undefined
          }

          switch (arg.mode) {
            case 'signAllTransactionsRequest': {
              const { requests} = await getPendingSignAllTransactionsRequests()

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
              return {
                error: `unsupported SVM simulation mode: ${arg.mode}`
              }
            }
          }

          const request = params[arg.mode]
          if (!request) {
            console.log(`unable to find ${arg.mode} Id: ${arg.id}`)
            return {
              error: 'request/transaction not found'
            }
          }

          const { result } = await simulationService.hasTransactionScanSupport(
            request.chainId,
            CoinTypes.SOL
          )

          if (!result) {
            throw new Error(
              `transaction simulation not supported for chain/coin: ${
                request.chainId //
              }/${CoinTypes.SOL}`
            )
          }

          const { errorResponse, errorString, response } =
            await simulationService.scanSolanaTransaction(
              toMojoUnion(params, arg.mode),
              navigator.language
            )

          if (errorResponse) {
            const error = `${errorString}: ${errorResponse}`
            const message = `Transaction Simulation error: ${error}`
            console.log(message)
            return {
              error
            }
          }

          if (!response) {
            throw new Error('empty simulation response')
          }

          return {
            data: response
          }
        } catch (error) {
          const message = `Failed to simulate transaction: ${error}`
          console.log(message)
          return {
            error: message
          }
        }
      }
    })
  }
}
