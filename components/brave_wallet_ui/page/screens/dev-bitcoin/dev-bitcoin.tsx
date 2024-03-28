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
  align-items: flex-start;
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

interface GetBalanceSectionProps {
  accountId: BraveWallet.AccountId
}

const GetBalanceSection = (props: GetBalanceSectionProps) => {
  const [loading, setLoading] = useState<boolean>(true)
  const [balance, setBalance] = useState<
    BraveWallet.BitcoinBalance | undefined
  >()

  // methods
  const fetchBalance = React.useCallback(async () => {
    setLoading(true)
    const result = await getAPIProxy().bitcoinWalletService.getBalance(
      props.accountId
    )
    setBalance(result.balance || undefined)
    setLoading(false)
  }, [props.accountId])

  // effects
  React.useEffect(() => {
    fetchBalance()
  }, [fetchBalance])

  // render
  return (
    <BalanceSection>
      <h2>getBalance</h2>
      {loading ? (
        <LoadingSkeleton
          useLightTheme={true}
          width={300}
          height={100}
        />
      ) : (
        <>
          <button onClick={fetchBalance}>Reload</button>
          <h3>balance: {balance?.totalBalance.toString()}</h3>
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

  const fetchBitcoinAccountInfo = React.useCallback(async () => {
    setLoading(true)
    const result =
      await getAPIProxy().bitcoinWalletService.getBitcoinAccountInfo(accountId)
    setBitcoinAccountInfo(result.accountInfo || undefined)
    setLoading(false)
  }, [accountId])

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

  // effects
  React.useEffect(() => {
    fetchBitcoinAccountInfo()
  }, [fetchBitcoinAccountInfo])

  // render
  return (
    <BitcoinAccountInfoSection>
      <h2>getBitcoinAccountInfo</h2>
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
              Run discovery
            </button>
          </div>
          <div>
            <code>Next Change Address: </code>
            <code>{keyId(bitcoinAccountInfo?.nextChangeAddress.keyId)} </code>
            <code>
              {bitcoinAccountInfo?.nextChangeAddress.addressString || '-'}
            </code>
            <button onClick={() => onRunDiscoverClick(true)}>
              Run discovery
            </button>
          </div>
        </>
      )}
    </BitcoinAccountInfoSection>
  )
}

interface AccountSectionProps {
  accountInfo: BraveWallet.AccountInfo
}

const AccountSection = (props: AccountSectionProps) => {
  return (
    <StyledWrapper>
      <h1>{props.accountInfo.name}</h1>
      <GetBitcoinAccountInfoSection
        accountId={props.accountInfo.accountId}
      ></GetBitcoinAccountInfoSection>
      <GetBalanceSection
        accountId={props.accountInfo.accountId}
      ></GetBalanceSection>
    </StyledWrapper>
  )
}

export const DevBitcoin = () => {
  const [accounts, setAccounts] = useState<BraveWallet.AccountInfo[]>([])

  React.useEffect(() => {
    const fetchBitcoinAccount = async () => {
      const allAccounts = (await getAPIProxy().keyringService.getAllAccounts())
        .allAccounts
      setAccounts(
        allAccounts.accounts.filter(
          (acc) => acc.accountId.coin === BraveWallet.CoinType.BTC
        )
      )
    }

    fetchBitcoinAccount()
  }, [])

  return (
    <div>
      {accounts.map((account) => (
        <div key={account.accountId.uniqueKey}>
          <AccountSection accountInfo={account} />
          <hr />
        </div>
      ))}
    </div>
  )
}

export default DevBitcoin
