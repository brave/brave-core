// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import type {
  BraveWallet,
  SerializableSolanaTxData,
  SerializableSolanaTxDataSendOptions,
  SerializableTimeDelta,
  SerializableTransactionInfo,
  SerializableTxDataUnion,
  TimeDelta
} from '../constants/types'

export function makeSerializableTimeDelta(
  td: TimeDelta | SerializableTimeDelta
): SerializableTimeDelta {
  return {
    microseconds: Number(td.microseconds)
  }
}

export function deserializeTimeDelta(td: SerializableTimeDelta): TimeDelta {
  return {
    microseconds: BigInt(td.microseconds)
  }
}

export function makeSerializableSolanaTxDataSendOptions(
  solanaTxData: BraveWallet.SolanaTxData
): SerializableSolanaTxDataSendOptions {
  if (!solanaTxData.sendOptions) {
    return undefined
  }

  return {
    ...solanaTxData.sendOptions,
    maxRetries: solanaTxData.sendOptions?.maxRetries
      ? {
          maxRetries: Number(solanaTxData.sendOptions.maxRetries)
        }
      : undefined
  }
}

export function makeSerializableSolanaTxData(
  solanaTxData: BraveWallet.SolanaTxData
): SerializableSolanaTxData {
  return {
    ...solanaTxData,
    lastValidBlockHeight: String(solanaTxData?.lastValidBlockHeight),
    lamports: String(solanaTxData?.lamports),
    amount: String(solanaTxData?.amount),
    sendOptions: solanaTxData.sendOptions
      ? {
          ...solanaTxData.sendOptions,
          maxRetries: solanaTxData.sendOptions?.maxRetries
            ? {
                maxRetries: Number(
                  solanaTxData.sendOptions.maxRetries.maxRetries
                )
              }
            : undefined
        }
      : undefined
  }
}

export function deserializeSolanaTxData(
  solanaTxData: SerializableSolanaTxData
): BraveWallet.SolanaTxData {
  return {
    ...solanaTxData,
    lastValidBlockHeight: BigInt(solanaTxData?.lastValidBlockHeight),
    lamports: BigInt(solanaTxData?.lamports),
    amount: BigInt(solanaTxData?.amount),
    sendOptions: solanaTxData.sendOptions
      ? {
          ...solanaTxData.sendOptions,
          maxRetries: solanaTxData.sendOptions?.maxRetries
            ? {
                maxRetries: BigInt(
                  solanaTxData.sendOptions.maxRetries.maxRetries
                )
              }
            : undefined
        }
      : undefined
  }
}

export function makeSerializableTransaction(
  tx: BraveWallet.TransactionInfo
): SerializableTransactionInfo {
  return {
    ...tx,
    txDataUnion: tx.txDataUnion.solanaTxData
      ? {
          solanaTxData: makeSerializableSolanaTxData(
            tx.txDataUnion.solanaTxData
          )
        }
      : (tx.txDataUnion as SerializableTxDataUnion),
    confirmedTime: makeSerializableTimeDelta(tx.confirmedTime),
    createdTime: makeSerializableTimeDelta(tx.createdTime),
    submittedTime: makeSerializableTimeDelta(tx.submittedTime)
  }
}

export function deserializeTransaction(
  tx: SerializableTransactionInfo
): BraveWallet.TransactionInfo {
  const txDataUnion = tx.txDataUnion.solanaTxData
    ? { solanaTxData: deserializeSolanaTxData(tx.txDataUnion.solanaTxData) }
    : tx.txDataUnion

  return {
    ...tx,
    txDataUnion: txDataUnion as BraveWallet.TxDataUnion,
    confirmedTime: deserializeTimeDelta(tx.confirmedTime),
    createdTime: deserializeTimeDelta(tx.createdTime),
    submittedTime: deserializeTimeDelta(tx.submittedTime)
  }
}
