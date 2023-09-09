// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getAPIProxy from '../../../common/async/bridge'
import * as React from 'react'

import styled from 'styled-components'
import { useState } from 'react'
import { BraveWallet } from '../../../constants/types'
import {
  LoadingSkeleton //
} from '../../../components/shared/loading-skeleton/index'

const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: center;
  width: 100%;
  padding-top: 32px;
`

const AccountInfoSection = styled.div`
  margin: 10px;
  padding: 5px;
`
const BalanceSection = styled.div`
  margin: 10px;
  padding: 5px;
`

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
    const { accountInfo } = await getAPIProxy().keyringService.addAccount(
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

interface GetBalanceSectionProps {
  accountId: BraveWallet.AccountId
}

const GetBalanceSection = (props: GetBalanceSectionProps) => {
  const [loading, setLoading] = useState<boolean>(true)
  const [balance, setBalance] = useState<
    BraveWallet.BitcoinBalance | undefined
  >()

  React.useEffect(() => {
    fetchBalance()
  }, [])

  const fetchBalance = async () => {
    setLoading(true)
    const result = await getAPIProxy().bitcoinWalletService.getBalance(
      defaultNetworkId,
      props.accountId
    )
    setBalance(result.balance || undefined)
    setLoading(false)
  }

  return (
    <BalanceSection>
      <h1>getBalance</h1>
      {loading ? (
        <LoadingSkeleton useLightTheme={true} width={300} height={100} />
      ) : (
        <>
          <button onClick={fetchBalance}>Reload</button>
          <h2>balance: {balance?.totalBalance.toString()}</h2>
          <ul>
            {balance?.balances &&
              Object.entries(balance.balances as { [key: string]: BigInt }).map(
                ([address, balance]) => {
                  return (
                    <li key={address}>
                      <AddressLine>
                        <Address>{address}</Address>
                        <Balance>{balance.toString()}</Balance>
                      </AddressLine>
                    </li>
                  )
                }
              )}
          </ul>
        </>
      )}
    </BalanceSection>
  )
}

interface GetBitcoinAccountInfoSectionProps {
  accountId: BraveWallet.AccountId
}

const GetBitcoinAccountInfoSection = (
  props: GetBitcoinAccountInfoSectionProps
) => {
  const [loading, setLoading] = useState<boolean>(true)
  const [accountInfo, setAccountInfo] = useState<
    BraveWallet.BitcoinAccountInfo | undefined
  >()

  const fetchAccountInfo = async () => {
    setLoading(true)

    const accountInfo =
      await getAPIProxy().bitcoinWalletService.getBitcoinAccountInfo(
        defaultNetworkId,
        props.accountId
      )
    setAccountInfo(accountInfo.accountInfo || undefined)
    setLoading(false)
  }

  React.useEffect(() => {
    fetchAccountInfo()
  }, [])

  return (
    <AccountInfoSection>
      <h1>getBitcoinAccountInfo</h1>
      {loading ? (
        <LoadingSkeleton useLightTheme={true} width={300} height={100} />
      ) : (
        <>
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
                      <Balance>
                        {a.utxoList !== null ? a.balance.toString() : 'N/A'}
                      </Balance>
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
                      <Balance>
                        {a.utxoList !== null ? a.balance.toString() : 'N/A'}
                      </Balance>
                    </AddressLine>
                  </li>
                )
              })}
          </ul>
        </>
      )}
    </AccountInfoSection>
  )
}

interface AccountSectionProps {
  accountId: BraveWallet.AccountId
}

const AccountSection = (props: AccountSectionProps) => {
  const [sendToAddress, setSendToAddress] = useState<string>('')

  const [sendToAmount, setSendToAmount] = useState<number>(1000)
  const [sendToFee, setSendToFee] = useState<number>(300)
  const [sending, setSending] = useState<boolean>(false)
  const [sendResult, setSendResult] = useState<
    | {
        txid: string
        error: string
      }
    | undefined
  >()

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

  return (
    <StyledWrapper>
      <GetBalanceSection accountId={props.accountId}></GetBalanceSection>
      <GetBitcoinAccountInfoSection
        accountId={props.accountId}
      ></GetBitcoinAccountInfoSection>

      <SendToSection>
        <h2>Send to:</h2>
        <SendToLine>
          <label>Address:</label>
          <input
            style={{ width: '35em', marginLeft: '1em' }}
            type='text'
            value={sendToAddress}
            onChange={handlesendToAddressInputChanged}
          ></input>
        </SendToLine>
        <SendToLine>
          <label>Amount:</label>
          <input
            style={{ width: '35em', marginLeft: '1em' }}
            type='number'
            value={sendToAmount}
            onChange={handlesendToAmountInputChanged}
          ></input>
        </SendToLine>
        <SendToLine>
          <label>Fee:</label>
          <input
            style={{ width: '35em', marginLeft: '1em' }}
            type='number'
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
    <CreateAccountSection setAccountId={setAccountId} />
  )
}

export default DevBitcoin
