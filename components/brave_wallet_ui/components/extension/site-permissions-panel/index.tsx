import * as React from 'react'
import { WalletAccountType } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import {
  ConnectHeader,
  DividerLine,
  ConnectedAccountItem
} from '../'

// Styled Components
import {
  StyledWrapper,
  AddressContainer,
  AddressScrollContainer,
  AccountsTitle
} from './style'
// import { getLocale } from '../../../../common/locale'

export interface Props {
  onDisconnect: (origin: string, address: string) => void
  siteURL: string
  connectedAccounts: WalletAccountType[]
}

const SitePermissions = (props: Props) => {
  const { siteURL, connectedAccounts, onDisconnect } = props

  const onDisconnectFromOrigin = (address: string) => {
    onDisconnect(siteURL, address)
  }

  return (
    <StyledWrapper>
      <ConnectHeader hideTitle={true} url={siteURL} />
      <AddressContainer>
        <AccountsTitle>{connectedAccounts.length} {getLocale('braveWalletSitePermissionsAccounts')}</AccountsTitle>
        {connectedAccounts.length !== 0 &&
          <>
            <DividerLine />
            <AddressScrollContainer>
              {connectedAccounts.map((account) => (
                <ConnectedAccountItem
                  key={account.id}
                  account={account}
                  onDisconnect={onDisconnectFromOrigin}
                />
              ))}
            </AddressScrollContainer>
            <DividerLine />
          </>
        }
      </AddressContainer>
    </StyledWrapper>
  )
}

export default SitePermissions
