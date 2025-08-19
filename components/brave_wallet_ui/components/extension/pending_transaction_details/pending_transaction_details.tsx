// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { EntityState } from '@reduxjs/toolkit'

// Slices
import {
  useGetAccountInfosRegistryQuery, //
} from '../../../common/slices/api.slice'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
} from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { getTransactionTypeName } from '../../../utils/tx-utils'
import { numberArrayToHexStr } from '../../../utils/hex-utils'
import { findAccountByAddress } from '../../../utils/account-utils'
import { getSolanaProgramIdName } from '../../../utils/solana-program-id-utils'
import {
  formatSolInstructionParamValue,
  TypedSolanaInstructionWithParams,
} from '../../../utils/solana-instruction-utils'

// Styled Components
import {
  StyledWrapper,
  DetailColumn,
  LabelText,
  DetailText,
  NoDataText,
} from './pending_transaction_details.styles'
import { Column, VerticalDivider } from '../../shared/style'

interface Props {
  transactionInfo: SerializableTransactionInfo
  instructions?: TypedSolanaInstructionWithParams[]
}

export function PendingTransactionDetails(props: Props) {
  const { transactionInfo, instructions } = props
  const { txArgs, txParams, txType, txDataUnion } = transactionInfo

  // Computed
  const solData = txDataUnion.solanaTxData
  const sendOptions = solData?.sendOptions
  const btcData = txDataUnion.btcTxData
  const zecData = txDataUnion.zecTxData
  const adaData = txDataUnion.cardanoTxData
  const btcZecOrAdaData = btcData || zecData || adaData
  const dataArray = txDataUnion.ethTxData1559?.baseData.data || []

  // BTC, ZEC, Cardano
  if (btcZecOrAdaData) {
    return (
      <StyledWrapper>
        {btcZecOrAdaData.inputs?.map((input, index) => {
          return (
            <DetailColumn
              gap='8px'
              key={'input' + index}
            >
              <DetailColumn>
                <LabelText>{getLocale('braveWalletInput')}:</LabelText>
                <DetailText>{`${index}`}</DetailText>
              </DetailColumn>
              <DetailColumn>
                <LabelText>{getLocale('braveWalletValue')}:</LabelText>
                <DetailText>{`${input.value}`}</DetailText>
              </DetailColumn>
              <DetailColumn>
                <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
                <DetailText>{`${input.address}`}</DetailText>
              </DetailColumn>
              <VerticalDivider />
            </DetailColumn>
          )
        })}
        {btcZecOrAdaData.outputs?.map((output, index) => {
          return (
            <DetailColumn
              gap='8px'
              key={'output' + index}
            >
              <DetailColumn>
                <LabelText>{getLocale('braveWalletOutput')}:</LabelText>
                <DetailText>{`${index}`}</DetailText>
              </DetailColumn>
              <DetailColumn>
                <LabelText>{getLocale('braveWalletValue')}:</LabelText>
                <DetailText>{`${output.value}`}</DetailText>
              </DetailColumn>
              <DetailColumn>
                <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
                <DetailText>{`${output.address}`}</DetailText>
              </DetailColumn>
              <VerticalDivider />
            </DetailColumn>
          )
        })}
      </StyledWrapper>
    )
  }

  // No Data
  if (dataArray.length === 0 && !solData) {
    return (
      <StyledWrapper>
        <Column
          width='100%'
          padding='24px'
        >
          <NoDataText>
            {getLocale('braveWalletConfirmTransactionNoData')}
          </NoDataText>
        </Column>
      </StyledWrapper>
    )
  }

  // SOL
  if (solData || sendOptions) {
    return (
      <StyledWrapper
        width='100%'
        padding='16px'
        gap='8px'
      >
        {/* SOL Options */}
        {sendOptions && (
          <>
            {!!Number(sendOptions?.maxRetries?.maxRetries) && (
              <DetailColumn key={'maxRetries'}>
                <LabelText>
                  {getLocale('braveWalletSolanaMaxRetries')}
                </LabelText>
                <DetailText>{sendOptions?.maxRetries?.maxRetries}</DetailText>
              </DetailColumn>
            )}
            {sendOptions?.preflightCommitment && (
              <DetailColumn key={'preflightCommitment'}>
                <LabelText>
                  {getLocale('braveWalletSolanaPreflightCommitment')}
                </LabelText>
                <DetailText>{sendOptions?.preflightCommitment}</DetailText>
              </DetailColumn>
            )}
            {sendOptions?.skipPreflight && (
              <DetailColumn key={'skipPreflight'}>
                <LabelText>
                  {getLocale('braveWalletSolanaSkipPreflight')}
                </LabelText>
                <DetailText>
                  {sendOptions.skipPreflight.skipPreflight.toString()}
                </DetailText>
              </DetailColumn>
            )}
          </>
        )}
        {/* Function */}
        {(solData || dataArray) && (
          <DetailColumn gap='8px'>
            <DetailColumn>
              <LabelText>
                {getLocale('braveWalletTransactionDetailBoxFunction')}:
              </LabelText>
              <DetailText>{getTransactionTypeName(txType)}</DetailText>
            </DetailColumn>
            <VerticalDivider />
          </DetailColumn>
        )}
        {/* SOL Instructions */}
        {instructions?.map((instruction, index) => {
          return (
            <SolanaTransactionInstruction
              key={index}
              typedInstructionWithParams={instruction}
            />
          )
        })}
      </StyledWrapper>
    )
  }

  // EVM & FIL
  return (
    <StyledWrapper
      width='100%'
      padding='16px'
      gap='8px'
    >
      {/* Function */}
      {dataArray && (
        <DetailColumn>
          <LabelText>
            {getLocale('braveWalletTransactionDetailBoxFunction')}:
          </LabelText>
          <DetailText>{getTransactionTypeName(txType)}</DetailText>
        </DetailColumn>
      )}
      {txType === BraveWallet.TransactionType.Other ? (
        <DetailColumn>
          <DetailText>{`0x${numberArrayToHexStr(dataArray)}`}</DetailText>
        </DetailColumn>
      ) : (
        txParams.map((param, i) => (
          <DetailColumn
            gap='8px'
            key={i}
          >
            <DetailColumn>
              <LabelText>{param}:</LabelText>
              <DetailText>{txArgs[i]}</DetailText>
            </DetailColumn>
          </DetailColumn>
        ))
      )}
    </StyledWrapper>
  )
}

export default PendingTransactionDetails

interface SolanaInstructionProps {
  typedInstructionWithParams: TypedSolanaInstructionWithParams
}

const SolanaTransactionInstruction: React.FC<SolanaInstructionProps> = ({
  typedInstructionWithParams: {
    accountParams,
    data,
    programId,
    type,
    params,
    accountMetas,
  },
}) => {
  const { data: accounts } = useGetAccountInfosRegistryQuery()

  // render
  if (!type) {
    // instruction not decodable by backend
    return (
      <DetailColumn gap='8px'>
        {programId && (
          <DetailColumn>
            <LabelText>{getLocale('braveWalletSolanaProgramID')}</LabelText>
            <DetailText>{JSON.stringify(programId)}</DetailText>
          </DetailColumn>
        )}

        {accountMetas.length > 0 && (
          <>
            <DetailColumn>
              <LabelText>{getLocale('braveWalletSolanaAccounts')}</LabelText>

              {accountMetas.map(({ pubkey, addrTableLookupIndex }, i) => {
                // other account metas
                return (
                  <AddressParamValue
                    key={`${pubkey}-${i}`}
                    accounts={accounts}
                    pubkey={pubkey}
                    lookupTableIndex={addrTableLookupIndex?.val}
                  />
                )
              })}
            </DetailColumn>
          </>
        )}

        {data && (
          <>
            <DetailColumn>
              <LabelText>{getLocale('braveWalletSolanaData')}</LabelText>
              <DetailText>{JSON.stringify(data)}</DetailText>
            </DetailColumn>
          </>
        )}
        <VerticalDivider />
      </DetailColumn>
    )
  }

  // Decoded instructions
  return (
    <DetailColumn gap='8px'>
      <DetailColumn>
        <LabelText>{getSolanaProgramIdName(programId)}:</LabelText>
        <DetailText>{type}</DetailText>
      </DetailColumn>

      {accountParams.length > 0 && (
        <>
          {accountParams.map(({ localizedName, name, value }, i) => {
            // signers param
            if (name === BraveWallet.SIGNERS) {
              if (!value) {
                return null
              }

              const signers = value.split(',')
              if (!signers.length) {
                return null
              }

              return (
                <DetailColumn key={name}>
                  <LabelText>{localizedName}</LabelText>
                  {signers.map((pubkey) => {
                    return (
                      <AddressParamValue
                        key={pubkey}
                        accounts={accounts}
                        pubkey={pubkey}
                      />
                    )
                  })}
                </DetailColumn>
              )
            }

            // other account params
            return (
              <DetailColumn key={name}>
                <LabelText>{localizedName}</LabelText>
                <AddressParamValue
                  accounts={accounts}
                  pubkey={value}
                  lookupTableIndex={accountMetas[i]?.addrTableLookupIndex?.val}
                />
              </DetailColumn>
            )
          })}
        </>
      )}

      {params.length > 0 && (
        <DetailColumn gap='8px'>
          <VerticalDivider />
          {params.map((param) => {
            const { localizedName, name, value } = param
            const { formattedValue, valueType } =
              formatSolInstructionParamValue(param, accounts)

            const isAddressParam = valueType === 'address'

            return (
              <DetailColumn key={name}>
                <LabelText>{localizedName}</LabelText>
                {isAddressParam ? (
                  <AddressParamValue
                    accounts={accounts}
                    pubkey={value}
                  />
                ) : (
                  <DetailText>{formattedValue}</DetailText>
                )}
              </DetailColumn>
            )
          })}
        </DetailColumn>
      )}
    </DetailColumn>
  )
}

const AddressParamValue = ({
  accounts,
  pubkey,
  lookupTableIndex,
}: {
  accounts: EntityState<BraveWallet.AccountInfo> | undefined
  pubkey: string
  lookupTableIndex?: number
}) => {
  // memos
  const formattedValue = React.useMemo(() => {
    return findAccountByAddress(pubkey, accounts)?.name ?? pubkey
  }, [accounts, pubkey])

  // render
  return (
    <DetailColumn gap='8px'>
      <DetailColumn>
        {lookupTableIndex !== undefined && (
          <LabelText>
            {getLocale('braveWalletSolanaAddressLookupTableAccount')}
          </LabelText>
        )}
        <DetailText>{formattedValue}</DetailText>
      </DetailColumn>
      {lookupTableIndex !== undefined && (
        <DetailColumn>
          <LabelText>
            {getLocale('braveWalletSolanaAddressLookupTableIndex')}
          </LabelText>
          <DetailText>{lookupTableIndex}</DetailText>
        </DetailColumn>
      )}
    </DetailColumn>
  )
}
