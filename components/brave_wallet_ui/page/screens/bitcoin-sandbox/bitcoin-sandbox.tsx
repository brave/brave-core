// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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

const keyringId = BraveWallet.BITCOIN_TESTNET_KEYRING_ID
const accountIndex = 0

export const BitcoinSandbox = () => {
  //  const [utxoList, setUtxoList] = useState<string>('')
  // const [receivingAddresses, setReceivingAddresses] = useState<
  //   BitcoinAddressInfo[]
  // >([])
  // const [changeAddresses, setChangeAddresses] = useState<BitcoinAddressInfo[]>(
  //   []
  // )

  const [loading, setLoading] = useState<boolean>(true)

  const [accountInfo, setAccountInfo] =
    useState<BraveWallet.BitcoinAccountInfo | null>(null)

  const [sendToAddress, setSendToAddress] = useState<string>(
    'tb1qamllhrjtvfsk2j78urzmmh4fa9dyyygy2gxc9m'
  )

  const [sendToAmount, setSendToAmount] = useState<number>(1000)
  const [sendToFee, setSendToFee] = useState<number>(300)
  const [sending, setSending] = useState<boolean>(false)
  const [sendResult, setSendResult] = useState<{
    txid: string
    url: string
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
      await getAPIProxy().bitcoinRpcService.getBitcoinAccountInfo(
        keyringId,
        accountIndex
      )
    setAccountInfo(accountInfo.accountInfo)
    setLoading(false)
  }

  const doSend = async () => {
    setSendResult(null)
    setSending(true)
    const result = await getAPIProxy().bitcoinRpcService.sendTo(
      keyringId,
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
        <h2>Balance: {accountInfo?.balance}</h2>
        <AddressesHeader>Receiving Addresses</AddressesHeader>
        <ul>
          {accountInfo?.addressInfos
            .filter((a) => !a.address.id.change)
            .map((a) => {
              return (
                <li key={a.address.address}>
                  <AddressLine>
                    <Address>{a.address.address}</Address>
                    <Balance>{a.balance}</Balance>
                  </AddressLine>
                </li>
              )
            })}
        </ul>
        <AddressesHeader>Change Addresses</AddressesHeader>
        <ul>
          {accountInfo?.addressInfos
            .filter((a) => a.address.id.change)
            .map((a) => {
              return (
                <li key={a.address.address}>
                  <AddressLine>
                    <Address>{a.address.address}</Address>
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
        {sendResult && sendResult.txid && (
          <a target="_blank" href={sendResult?.url}>
            {sendResult?.txid}
          </a>
        )}
      </SendToSection>
    </StyledWrapper>
  )
}

export default BitcoinSandbox
