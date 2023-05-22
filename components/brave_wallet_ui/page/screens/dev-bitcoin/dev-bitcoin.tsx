// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getAPIProxy from '../../../common/async/bridge'
import * as React from 'react'

// Redux
import { useSelector, useDispatch } from 'react-redux'
import { WalletActions } from '../../../common/actions'

import styled from 'styled-components'
import { useState } from 'react'
import { BraveWallet, WalletState } from '../../../constants/types'
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
const defaultKeyringId = BraveWallet.BITCOIN_KEYRING84_TEST_ID
const accountIndex = 0

const CreateAccountSection = () => {
  const dispatch = useDispatch()

  const createBtcAccount = async () => {
    dispatch(
      WalletActions.addBitcoinAccount({
        accountName: 'BTC Account',
        networkId: defaultNetworkId,
        keyringId: defaultKeyringId
      })
    )
  }

  return (
    <StyledWrapper>
      <button onClick={createBtcAccount}>Create Account</button>
    </StyledWrapper>
  )
}

const AccountSection = () => {
  const [loading, setLoading] = useState<boolean>(true)

  const [accountInfo, setAccountInfo] =
    useState<BraveWallet.BitcoinAccountInfo | null>(null)

  const [sendToAddress, setSendToAddress] = useState<string>('')

  const [sendToAmount, setSendToAmount] = useState<number>(1000)
  const [sendToFee, setSendToFee] = useState<number>(300)
  const [sending, setSending] = useState<boolean>(false)
  const [sendResult, setSendResult] = useState<{
    txid: string
    error: string
  } | null>(null)

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
        defaultKeyringId,
        accountIndex
      )
    setAccountInfo(accountInfo.accountInfo ?? null)
    setLoading(false)
  }

  const doSend = async () => {
    setSendResult(null)
    setSending(true)
    const result = await getAPIProxy().bitcoinWalletService.sendTo(
      defaultNetworkId,
      defaultKeyringId,
      accountIndex,
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
          {accountInfo?.name} balance: {accountInfo?.balance}
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
                    <Balance>{a.balance}</Balance>
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
                    <Balance>{a.balance}</Balance>
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
  const accounts = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.accounts
  )

  const hasBitcoinAccount =
    accounts.filter(
      (a) =>
        a.accountId.coin === BraveWallet.CoinType.BTC &&
        a.accountId.keyringId === defaultKeyringId
    ).length > 0

  return hasBitcoinAccount ? <AccountSection /> : <CreateAccountSection />
}

export default DevBitcoin
