// Copyright (c) 2024 The Brave Authors. All rights reserved.
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
import { CoinType, ZCashShieldSyncStatus } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m'
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'

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

const ZCashAccountInfoSection = styled.div`
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
    BraveWallet.ZCashBalance | undefined
  >()
  const [shieldResult, setShieldResult] = useState<string>()
  const [
    makeAccountShieldableResult,
    setMakeAccountShieldableResult] = useState<string>()
  const [syncStatusResult, setSyncStatusResult] = useState<string>();
  const [shieldedBalanceValue, setShieldedBalanceValue] = useState<string>();

  const makeAccountShielded = async() => {
    const result = await getAPIProxy().zcashWalletService.makeAccountShielded(
        props.accountId);
    setMakeAccountShieldableResult(result.errorMessage || 'Done');
  }

  // methods
  const startOrchardSync = async() => {
    setSyncStatusResult('')
    const result =
        await getAPIProxy().zcashWalletService.startShieldSync(props.accountId);

    if (result.errorMessage) {
      setSyncStatusResult("Sync error " + result.errorMessage);
    }
  }

  const stopOrchardSync = async() => {
    const result =
      await getAPIProxy().zcashWalletService.stopShieldSync(props.accountId);
    if (result.errorMessage) {
      setSyncStatusResult("Stop error " + result.errorMessage);
    }
  }

  const shieldFunds = async () => {
    let {
      txId,
      errorMessage,
    } = await getAPIProxy().zcashWalletService.shieldFunds(
      BraveWallet.Z_CASH_MAINNET,
      props.accountId
    )
    setShieldResult(txId || errorMessage || "unknown")
  }

  const fetchBalance = React.useCallback(async () => {
    setLoading(true)
    const result = await getAPIProxy().zcashWalletService.getBalance(
      BraveWallet.Z_CASH_MAINNET,
      props.accountId
    )
    setBalance(result.balance || undefined)
    setLoading(false)
  }, [props.accountId])

  // effects
  React.useEffect(() => {
    const zcashWalletServiceObserver =
        new BraveWallet.ZCashWalletServiceObserverReceiver({
      onSyncStart: (accountId: BraveWallet.AccountId) => {
        if (props.accountId.uniqueKey === accountId.uniqueKey) {
          setSyncStatusResult("Started");
        }
      },
      onSyncStop: (accountId: BraveWallet.AccountId) => {
        if (props.accountId.uniqueKey === accountId.uniqueKey) {
          setSyncStatusResult("Stopped");
        }
      },
      onSyncStatusUpdate: (accountId: BraveWallet.AccountId,
                           status: ZCashShieldSyncStatus) => {
        if (props.accountId.uniqueKey === accountId.uniqueKey) {
          setSyncStatusResult("Current block " +
                              status.currentBlock + "/" +
                              status.chainTip);
          setShieldedBalanceValue("Found balance: " + status.spendableBalance);
        }
      },
      onSyncError: (accountId: BraveWallet.AccountId, error: string) => {
        if (props.accountId.uniqueKey === accountId.uniqueKey) {
          setSyncStatusResult("Error : " + error);
        }
      }
    });

    getAPIProxy().zcashWalletService.addObserver(
        zcashWalletServiceObserver.$.bindNewPipeAndPassRemote());

    return () => zcashWalletServiceObserver.$.close()
  }, [props.accountId])

  React.useEffect(() => {
    fetchBalance()
  }, [fetchBalance])

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
          <button onClick={makeAccountShielded}>Upgrade to shielded</button>
          <button onClick={startOrchardSync}>Start orchard sync</button>
          <button onClick={stopOrchardSync}>Stop orchard sync</button>

          <button onClick={fetchBalance}>Reload</button>
          <button onClick={shieldFunds}>Shield</button>
          <h3>make account shieldable result: {makeAccountShieldableResult}</h3>
          <h3>sync status: {syncStatusResult}</h3>
          <h3>shield result: {shieldResult}</h3>
          <h3>balance: {balance?.totalBalance.toString()}</h3>
          <h3>shielded balance: {shieldedBalanceValue}</h3>

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

interface GetZCashAccountInfoSectionProps {
  accountId: BraveWallet.AccountId
}

const GetZCashAccountInfoSection: React.FC<
  GetZCashAccountInfoSectionProps
> = ({ accountId }) => {
  const [loading, setLoading] = useState<boolean>(true)
  const [zcashAccountInfo, setZCashAccountInfo] = useState<
    BraveWallet.ZCashAccountInfo | undefined
  >()

  // methods
  const fetchZCashAccountInfo = React.useCallback(async () => {
    setLoading(true)
    const result =
      await getAPIProxy().zcashWalletService.getZCashAccountInfo(accountId)
    setZCashAccountInfo(result.accountInfo || undefined)
    setLoading(false)
  }, [accountId])

  // effects
  React.useEffect(() => {
    fetchZCashAccountInfo()
  }, [fetchZCashAccountInfo])

  const keyId = (keyId: BraveWallet.BitcoinKeyId | undefined) => {
    if (!keyId) {
      return '-'
    }
    return `../${keyId.change}/${keyId.index}`
  }

  const onRunDiscoverClick = async (change: boolean) => {
    await getAPIProxy().bitcoinWalletService.runDiscovery(accountId, change)
    fetchZCashAccountInfo()
  }

  return (
    <ZCashAccountInfoSection>
      <h2>getZCashAccountInfo</h2>
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
            <code>{keyId(zcashAccountInfo?.nextTransparentReceiveAddress.keyId)}
            </code>
            <code>
              {zcashAccountInfo?.nextTransparentReceiveAddress.addressString || '-'}
            </code>
            <button onClick={() => onRunDiscoverClick(false)}>
              Run discovery
            </button>
          </div>
          <div>
            <code>Next Change Address: </code>
            <code>{keyId(
                zcashAccountInfo?.nextTransparentChangeAddress.keyId)}
            </code>
            <code>
              {zcashAccountInfo?.nextTransparentChangeAddress.addressString || '-'}
            </code>
            <button onClick={() => onRunDiscoverClick(true)}>
              Run discovery
            </button>
          </div>
        </>
      )}
    </ZCashAccountInfoSection>
  )
}

interface AccountSectionProps {
  accountInfo: BraveWallet.AccountInfo
}

const AccountSection = (props: AccountSectionProps) => {
  return (
    <StyledWrapper>
      <h1>{props.accountInfo.name}</h1>
      <GetZCashAccountInfoSection
        accountId={props.accountInfo.accountId}
      ></GetZCashAccountInfoSection>
      <GetBalanceSection
        accountId={props.accountInfo.accountId}
      ></GetBalanceSection>
    </StyledWrapper>
  )
}

export const DevZCash = () => {
  const { accounts } = useAccountsQuery()
  const zecAccounts = React.useMemo(() => {
    return accounts.filter(
      (account) => account.accountId.coin === CoinType.ZEC
    )
  }, [accounts])
  return (
    <div>
      {zecAccounts && zecAccounts.map((account) => (
        <div key={account.accountId.uniqueKey}>
          <AccountSection accountInfo={account} />
          <hr />
        </div>
      ))}
    </div>
  )
}

export default DevZCash
