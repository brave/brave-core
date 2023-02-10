// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  ObjWithOriginInfo,
  SerializableDecryptRequest,
  SerializableGetEncryptionPublicKeyRequest,
  SerializableOrigin,
  SerializableOriginInfo,
  SerializableSignMessageRequest,
  SerializableSolanaTxData,
  SerializableSolanaTxDataSendOptions,
  SerializableSwitchChainRequest,
  SerializableTimeDelta,
  SerializableTransactionInfo,
  SerializableUnguessableToken,
  TimeDelta,
  WithSerializableOriginInfo
} from '../constants/types'

export function makeSerializableTimeDelta (td: TimeDelta | SerializableTimeDelta): SerializableTimeDelta {
  return {
    microseconds: Number(td.microseconds)
  }
}

export function makeSerializableUnguessableToken (
  token:
    | BraveWallet.OriginInfo['origin']['nonceIfOpaque']
    | SerializableUnguessableToken
): SerializableUnguessableToken | undefined {
  if (!token) {
    return undefined
  }

  return {
    high: token.high.toString(),
    low: token.low.toString()
  }
}

export function deserializableUnguessableToken (token?: SerializableUnguessableToken): BraveWallet.OriginInfo['origin']['nonceIfOpaque'] {
  if (!token) {
    return undefined
  }

  return {
    high: BigInt(token.high),
    low: BigInt(token.low)
  }
}

type OriginInfo = BraveWallet.OriginInfo | SerializableOriginInfo

export function makeSerializableOriginInfo <
  T extends OriginInfo | undefined
> (originInfo: T): T extends OriginInfo ? SerializableOriginInfo : undefined {
  return (originInfo ? {
    ...originInfo,
    origin: {
      ...originInfo?.origin,
      nonceIfOpaque: makeSerializableUnguessableToken(originInfo?.origin.nonceIfOpaque)
    }
  } : undefined) as T extends OriginInfo ? SerializableOriginInfo : undefined
}

export function deserializeOrigin (origin: SerializableOrigin): BraveWallet.OriginInfo['origin'] {
  return {
    ...origin,
    nonceIfOpaque: deserializableUnguessableToken(origin.nonceIfOpaque)
  }
}

export function revertSerializableOriginInfoProp <
  G,
  T extends WithSerializableOriginInfo<ObjWithOriginInfo<G>>
> (obj: T) {
  return {
    ...obj,
    originInfo: {
      ...obj.originInfo,
      origin: deserializeOrigin(obj.originInfo.origin)
    }
  }
}

export function makeSerializableSolanaTxDataSendOptions (
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

export function makeSerializableSolanaTxData (
  solanaTxData: BraveWallet.SolanaTxData
): SerializableSolanaTxData {
  return {
    ...solanaTxData,
    lastValidBlockHeight: String(solanaTxData?.lastValidBlockHeight),
    lamports: String(solanaTxData?.lamports),
    amount: String(solanaTxData?.amount),
    sendOptions: solanaTxData.sendOptions ? {
      ...solanaTxData.sendOptions,
      maxRetries: solanaTxData.sendOptions?.maxRetries ? {
        maxRetries: Number(solanaTxData.sendOptions.maxRetries)
      } : undefined
    } : undefined
  }
}

export function makeSerializableTransaction (tx: BraveWallet.TransactionInfo): SerializableTransactionInfo {
  return {
    ...tx,
    originInfo: makeSerializableOriginInfo(tx.originInfo),
    txDataUnion: {
      ...tx.txDataUnion,
      solanaTxData: tx.txDataUnion.solanaTxData
        ? makeSerializableSolanaTxData(tx.txDataUnion.solanaTxData)
        : undefined
    },
    confirmedTime: makeSerializableTimeDelta(tx.confirmedTime),
    createdTime: makeSerializableTimeDelta(tx.createdTime),
    submittedTime: makeSerializableTimeDelta(tx.submittedTime)
  }
}

export const makeOriginInfoPropSerializable = <T extends ObjWithOriginInfo>(req: T): WithSerializableOriginInfo<T> => {
  return {
    ...req,
    originInfo: makeSerializableOriginInfo(req.originInfo)
  }
}

export const makeSerializableSignMessageRequest = (req: BraveWallet.SignMessageRequest): SerializableSignMessageRequest => {
  return makeOriginInfoPropSerializable(req)
}

export const makeSerializableGetEncryptionPublicKeyRequest = (req: BraveWallet.GetEncryptionPublicKeyRequest): SerializableGetEncryptionPublicKeyRequest => {
  return makeOriginInfoPropSerializable(req)
}

export const makeSerializableDecryptRequest = (req: BraveWallet.DecryptRequest): SerializableDecryptRequest => {
  return makeOriginInfoPropSerializable(req)
}

export const makeSerializableSwitchChainRequest = (req: BraveWallet.SwitchChainRequest): SerializableSwitchChainRequest => {
  return makeOriginInfoPropSerializable(req)
}
