import * as React from 'react'
import { BraveWallet, Origin, WalletAccountType } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'

// Components
import {
  DividerLine,
  ConnectedAccountItem
} from '../'
import { CreateSiteOrigin } from '../../shared'

// Styled Components
import {
  StyledWrapper,
  AddressContainer,
  AddressScrollContainer,
  AccountsTitle,
  SiteOriginTitle,
  HeaderRow,
  HeaderColumn,
  FavIcon,
  NewAccountButton
} from './style'

export interface Props {
  onDisconnect: (origin: Origin, address: string, connectedAccounts: WalletAccountType[]) => void
  onConnect: (origin: Origin, address: WalletAccountType) => void
  onSwitchAccount: (account: WalletAccountType) => void
  onAddAccount: () => void
  selectedAccount: WalletAccountType
  originInfo: BraveWallet.OriginInfo
  connectedAccounts: WalletAccountType[]
  accounts: WalletAccountType[]
}

const SitePermissions = (props: Props) => {
  const {
    originInfo,
    connectedAccounts,
    selectedAccount,
    accounts,
    onDisconnect,
    onConnect,
    onSwitchAccount,
    onAddAccount
  } = props

  const onDisconnectFromOrigin = (address: string) => {
    const newConnectedAccounts = connectedAccounts.filter((accounts) => accounts.address.toLowerCase() !== address.toLowerCase())
    onDisconnect(originInfo.origin, address, newConnectedAccounts)
  }

  const onConnectToOrigin = (account: WalletAccountType) => {
    onConnect(originInfo.origin, account)
  }

  const onClickSwitchAccount = (account: WalletAccountType) => {
    onSwitchAccount(account)
  }

  const checkForPermission = React.useCallback((address: string): boolean => {
    return connectedAccounts.some(account => account.address.toLowerCase() === address.toLowerCase())
  }, [connectedAccounts])

  const checkIsActive = React.useCallback((address: string): boolean => {
    return address.toLowerCase() === selectedAccount.address.toLowerCase()
  }, [selectedAccount])

  return (
    <StyledWrapper>
      <HeaderRow>
        <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`} />
        <HeaderColumn>
          <SiteOriginTitle>
            <CreateSiteOrigin
              originSpec={originInfo.originSpec}
              eTldPlusOne={originInfo.eTldPlusOne}
            />
          </SiteOriginTitle>
          <AccountsTitle>{getLocale('braveWalletSitePermissionsAccounts').replace('$1', connectedAccounts.length.toString())}</AccountsTitle>
        </HeaderColumn>
      </HeaderRow>
      <AddressContainer>
        <NewAccountButton onClick={onAddAccount}>{getLocale('braveWalletSitePermissionsNewAccount')}</NewAccountButton>
        <DividerLine />
        <AddressScrollContainer>
          {accounts.map((account) => (
            <ConnectedAccountItem
              isActive={checkIsActive(account.address)}
              key={account.id}
              account={account}
              onDisconnect={onDisconnectFromOrigin}
              onConnect={onConnectToOrigin}
              onSwitchAccount={onClickSwitchAccount}
              hasPermission={checkForPermission(account.address)}
            />
          ))}
        </AddressScrollContainer>
      </AddressContainer>
    </StyledWrapper>
  )
}

export default SitePermissions
