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

const BalanceSection = styled.div`
  margin: 10px;
  padding: 5px;
`

const BitcoinAccountInfoSection = styled.div`
  margin: 10px;
  padding: 5px;
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
      props.accountId
    )
    setBalance(result.balance || undefined)
    setLoading(false)
  }

  return (
    <BalanceSection>
      <h1>getBalance</h1>
      {loading ? (
        <LoadingSkeleton
          useLightTheme={true}
          width={300}
          height={100}
        />
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

const GetBitcoinAccountInfoSection: React.FC<
  GetBitcoinAccountInfoSectionProps
> = ({ accountId }) => {
  const [loading, setLoading] = useState<boolean>(true)
  const [bitcoinAccountInfo, setBitcoinAccountInfo] = useState<
    BraveWallet.BitcoinAccountInfo | undefined
  >()

  React.useEffect(() => {
    fetchBitcoinAccountInfo()
  }, [])

  const fetchBitcoinAccountInfo = async () => {
    setLoading(true)
    const result =
      await getAPIProxy().bitcoinWalletService.getBitcoinAccountInfo(accountId)
    setBitcoinAccountInfo(result.accountInfo || undefined)
    setLoading(false)
  }

  const keyId = (keyId: BraveWallet.BitcoinKeyId | undefined) => {
    if (!keyId) {
      return '-'
    }
    return `../${keyId.change}/${keyId.index}`
  }

  const onRunDiscoverClick = async (change: boolean) => {
    await getAPIProxy().bitcoinWalletService.runDiscovery(accountId, change)
    fetchBitcoinAccountInfo()
  }

  return (
    <BitcoinAccountInfoSection>
      <h1>getBitcoinAccountInfo</h1>
      {loading ? (
        <LoadingSkeleton
          useLightTheme={true}
          width={300}
          height={100}
        />
      ) : (
        <>
          <div>
            <code>Next Receive Address: </code>
            <code>{keyId(bitcoinAccountInfo?.nextReceiveAddress.keyId)}</code>
            <code>
              {bitcoinAccountInfo?.nextReceiveAddress.addressString || '-'}
            </code>
            <button onClick={() => onRunDiscoverClick(false)}>
              Run disovery
            </button>
          </div>
          <div>
            <code>Next Change Address: </code>
            <code>{keyId(bitcoinAccountInfo?.nextChangeAddress.keyId)} </code>
            <code>
              {bitcoinAccountInfo?.nextChangeAddress.addressString || '-'}
            </code>
            <button onClick={() => onRunDiscoverClick(true)}>
              Run disovery
            </button>
          </div>
        </>
      )}
    </BitcoinAccountInfoSection>
  )
}

interface AccountSectionProps {
  accountId: BraveWallet.AccountId
}

const AccountSection = (props: AccountSectionProps) => {
  return (
    <StyledWrapper>
      <GetBitcoinAccountInfoSection
        accountId={props.accountId}
      ></GetBitcoinAccountInfoSection>
      <GetBalanceSection accountId={props.accountId}></GetBalanceSection>
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
