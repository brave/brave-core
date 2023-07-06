// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getAPIProxy from '../../../common/async/bridge'
import * as React from 'react'

import styled from 'styled-components'
import { useState } from 'react'
import { BraveWallet } from '../../../constants/types'
import { LoadingSkeleton } from '../../../components/shared'

const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: space-evenly;
  align-items: flex-start;
  width: 100%;
  padding-top: 32px;
`

const AccountInfoSection = styled.div``

const AddressesHeader = styled.h2`
  margin-bottom: 0;
`

const AddressLine = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: start;
`

const Address = styled.div`
  font-family: monospace;
  margin-right: 30px;
`

const Balance = styled.div`
  font-family: monospace;
`

const SendToSection = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: start;
`

const SendToLine = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
`

const defaultNetworkId = BraveWallet.BITCOIN_TESTNET
const defaultKeyringId = BraveWallet.KeyringId.kBitcoin84Testnet

interface CreateAccountSectionProps {
  setAccountId: (accountId: BraveWallet.AccountId | undefined) => void
}

const CreateAccountSection = (props: CreateAccountSectionProps) => {
  const createBtcAccount = async () => {
    const {accountInfo} = await getAPIProxy().keyringService.addAccount(
      BraveWallet.CoinType.BTC,
      BraveWallet.KeyringId.kBitcoin84Testnet,
      'BTC Account'
    )

    if (accountInfo) {
      props.setAccountId(accountInfo.accountId)
    }
  }

  return (
    <StyledWrapper>
      <button onClick={createBtcAccount}>Create Account</button>
    </StyledWrapper>
  )
}

interface AccountSectionProps {
  accountId: BraveWallet.AccountId
}

const AccountSection = (props: AccountSectionProps) => {
  const [loading, setLoading] = useState<boolean>(true)

  const [accountInfo, setAccountInfo] =
    useState<BraveWallet.BitcoinAccountInfo | undefined>()

  const [sendToAddress, setSendToAddress] = useState<string>('')

  const [sendToAmount, setSendToAmount] = useState<number>(1000)
  const [sendToFee, setSendToFee] = useState<number>(300)
  const [sending, setSending] = useState<boolean>(false)
  const [sendResult, setSendResult] = useState<{
    txid: string
    error: string
  } | undefined>()

  const handlesendToAddressInputChanged = ({
    target: { value }
  }: React.ChangeEvent<HTMLInputElement>) => {
    setSendToAddress(value)
  }

  const handlesendToAmountInputChanged = ({
    target: { value }
  }: React.ChangeEvent<HTMLInputElement>) => {
    setSendToAmount(Number(value))
  }

  const handlesendToFeeInputChanged = ({
    target: { value }
  }: React.ChangeEvent<HTMLInputElement>) => {
    setSendToFee(Number(value))
  }

  const fetchAccountInfo = async () => {
    const accountInfo =
      await getAPIProxy().bitcoinWalletService.getBitcoinAccountInfo(
        defaultNetworkId,
        props.accountId
      )
    setAccountInfo(accountInfo.accountInfo || undefined)
    setLoading(false)
  }

  const doSend = async () => {
    setSendResult(undefined)
    setSending(false)

    setSending(true)
    const result = await getAPIProxy().bitcoinWalletService.sendTo(
      defaultNetworkId,
      props.accountId,
      sendToAddress,
      BigInt(sendToAmount),
      BigInt(sendToFee)
    )
    setSending(false)
    setSendResult(result)
  }

  React.useEffect(() => {
    fetchAccountInfo()
  }, [])

  if (loading) {
    return (
      <StyledWrapper>
        <LoadingSkeleton useLightTheme={true} width={300} height={500} />
      </StyledWrapper>
    )
  }

  return (
    <StyledWrapper>
      <AccountInfoSection>
        <button onClick={fetchAccountInfo}>Reload</button>
        <h2>
          {accountInfo?.name} balance: {accountInfo?.balance.toString()}
        </h2>
        <AddressesHeader>Receiving Addresses</AddressesHeader>
        <ul>
          {accountInfo?.addressInfos
            .filter((a) => !a.keyId.change)
            .map((a) => {
              return (
                <li key={a.addressString}>
                  <AddressLine>
                    <Address>{a.addressString}</Address>
                    <Balance>{a.balance.toString()}</Balance>
                  </AddressLine>
                </li>
              )
            })}
        </ul>
        <AddressesHeader>Change Addresses</AddressesHeader>
        <ul>
          {accountInfo?.addressInfos
            .filter((a) => a.keyId.change)
            .map((a) => {
              return (
                <li key={a.addressString}>
                  <AddressLine>
                    <Address>{a.addressString}</Address>
                    <Balance>{a.balance.toString()}</Balance>
                  </AddressLine>
                </li>
              )
            })}
        </ul>
      </AccountInfoSection>
      <SendToSection>
        <h2>Send to:</h2>
        <SendToLine>
          <label>Address:</label>
          <input
            style={{ width: '35em', marginLeft: '1em' }}
            type="text"
            value={sendToAddress}
            onChange={handlesendToAddressInputChanged}
          ></input>
        </SendToLine>
        <SendToLine>
          <label>Amount:</label>
          <input
            style={{ width: '35em', marginLeft: '1em' }}
            type="number"
            value={sendToAmount}
            onChange={handlesendToAmountInputChanged}
          ></input>
        </SendToLine>
        <SendToLine>
          <label>Fee:</label>
          <input
            style={{ width: '35em', marginLeft: '1em' }}
            type="number"
            value={sendToFee}
            onChange={handlesendToFeeInputChanged}
          ></input>
        </SendToLine>
        <button disabled={sending} onClick={doSend}>
          Send
        </button>
        {sendResult && sendResult.txid && <span>{sendResult.txid}</span>}
        {sendResult && sendResult.error && <span>{sendResult.error}</span>}
      </SendToSection>
    </StyledWrapper>
  )
}

export const DevBitcoin = () => {
  const [accountId, setAccountId] = useState<
    BraveWallet.AccountId | undefined
  >()

  React.useEffect(() => {
    const fetchBitcoinAccount = async () => {
      const { accounts } = (await getAPIProxy().keyringService.getAllAccounts())
        .allAccounts
      const bitcoinAccount = accounts.find(
        (acc) => acc.accountId.keyringId === defaultKeyringId
      )
      setAccountId(bitcoinAccount?.accountId)
    }

    fetchBitcoinAccount()
  }, [])

  return accountId ? (
    <AccountSection accountId={accountId} />
  ) : (
    <CreateAccountSection setAccountId={setAccountId}/>
  )
}

export default DevBitcoin
