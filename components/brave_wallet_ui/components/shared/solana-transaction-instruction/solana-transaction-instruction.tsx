// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getSolanaProgramIdName } from '../../../utils/solana-program-id-utils'
import { getLocale } from '../../../../common/locale'
import { findAccountName } from '../../../utils/account-utils'
import { useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'

// types
import { BraveWallet } from '../../../constants/types'
import {
  formatSolInstructionParamValue,
  TypedSolanaInstructionWithParams
} from '../../../utils/solana-instruction-utils'

// components
import { CopyTooltip } from '../copy-tooltip/copy-tooltip'

// styles
import {
  Divider,
  SectionRow,
  TransactionTitle
} from '../../extension/confirm-transaction-panel/style'

import {
  InstructionBox,
  InstructionParamBox,
  AddressText,
  CodeSectionTitle
} from './solana-transaction-instruction.style'

import {
  CodeSnippet,
  CodeSnippetText
} from '../../extension/transaction-box/style'

interface Props {
  typedInstructionWithParams: TypedSolanaInstructionWithParams
}

export const SolanaTransactionInstruction: React.FC<Props> = ({
  typedInstructionWithParams: {
    accountParams,
    data,
    programId,
    type,
    params,
    accountMetas
  }
}) => {
  // redux
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)

  // render
  if (!type) {
    // instruction not decodable by backend
    return (
      <InstructionBox>
        {programId && (
          <>
            <CodeSectionTitle>
              {getLocale('braveWalletSolanaProgramID')}
            </CodeSectionTitle>
            <CodeSnippet>
              <code>
                <CodeSnippetText>{JSON.stringify(programId)}</CodeSnippetText>
              </code>
            </CodeSnippet>
          </>
        )}

        {accountMetas.length > 0 && (
          <>
            <Divider />

            <CodeSectionTitle>
              {getLocale('braveWalletSolanaAccounts')}
            </CodeSectionTitle>

            {accountMetas.map(({ pubkey, addrTableLookupIndex }, i) => {
              // other account metas
              return (
                <InstructionParamBox key={`${pubkey}-${i}`}>
                  <AddressParamValue
                    accounts={accounts}
                    pubkey={pubkey}
                    lookupTableIndex={addrTableLookupIndex?.val}
                  />
                </InstructionParamBox>
              )
            })}
          </>
        )}

        {data && (
          <>
            <Divider />

            <CodeSectionTitle>
              {getLocale('braveWalletSolanaData')}
            </CodeSectionTitle>
            <CodeSnippet>
              <code>
                <CodeSnippetText>{JSON.stringify(data)}</CodeSnippetText>
              </code>
            </CodeSnippet>
          </>
        )}
      </InstructionBox>
    )
  }

  // Decoded instructions
  return (
    <InstructionBox>
      <>
        <SectionRow>
          <TransactionTitle>
            <CopyTooltip
              text={programId}
              tooltipText={programId}
              isAddress
              position="left"
            >
              {getSolanaProgramIdName(programId)} - {type}
            </CopyTooltip>
          </TransactionTitle>
        </SectionRow>

        {accountParams.length > 0 && (
          <>
            <Divider />

            {accountParams.map((
              { localizedName, name, value }, i) => {
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
                  <InstructionParamBox key={name}>
                    <var>{localizedName}</var>
                    {signers.map((pubkey) => {
                      return (
                        <AddressParamValue
                          key={pubkey}
                          accounts={accounts}
                          pubkey={pubkey}
                        />
                      )
                    })}
                  </InstructionParamBox>
                )
              }

              // other account params
              return (
                <InstructionParamBox key={name}>
                  <var>{localizedName}</var>
                  <AddressParamValue
                    accounts={accounts}
                    pubkey={value}
                    lookupTableIndex={
                      accountMetas[i].addrTableLookupIndex?.val
                    }
                  />
                </InstructionParamBox>
              )
            })}
          </>
        )}

        {params.length > 0 && (
          <>
            <Divider />
            {params.map((param) => {
              const { localizedName, name, value } = param
              const { formattedValue, valueType } =
                formatSolInstructionParamValue(param, accounts)

              const isAddressParam = valueType === 'address'

              return (
                <InstructionParamBox key={name}>
                  <var>{localizedName}</var>
                  {isAddressParam ? (
                    <AddressParamValue accounts={accounts} pubkey={value} />
                  ) : (
                    <samp>{formattedValue}</samp>
                  )}
                </InstructionParamBox>
              )
            })}
          </>
        )}
      </>
    </InstructionBox>
  )
}

export default SolanaTransactionInstruction

const AddressParamValue = ({
  accounts,
  pubkey,
  lookupTableIndex
}: {
  accounts: Array<{
    address: string
    name: string
  }>
  pubkey: string
  lookupTableIndex?: number
}) => {
  // memos
  const formattedValue = React.useMemo(() => {
    return findAccountName(accounts, pubkey) ?? pubkey
  }, [accounts, pubkey])

  // render
  return (
    <>
      {lookupTableIndex !== undefined &&
        <AddressText isBold={true}>
          {getLocale('braveWalletSolanaAddressLookupTableAccount')}
        </AddressText>
      }
      <CopyTooltip
        key={pubkey}
        text={pubkey}
        tooltipText={
          formattedValue === pubkey
            ? undefined
            : pubkey
        }
        isAddress
        position='left'
      >
        <AddressText>{formattedValue}</AddressText>
      </CopyTooltip>
      {
        lookupTableIndex !== undefined &&
        <>
          <AddressText isBold={true}>
            {getLocale('braveWalletSolanaAddressLookupTableIndex')}
          </AddressText>
          <AddressText>{lookupTableIndex}</AddressText>
        </>
      }
    </>
  )
}
