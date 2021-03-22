import * as React from 'react'

// Components
import {
  DividerLine,
  SelectAddress,
  ConnectBottomNav,
  ConnectHeader
} from '../'

// Styled Components
import {
  StyledWrapper,
  SelectAddressContainer,
  NewAccountTitle,
  SelectAddressScrollContainer
} from './style'

export interface WalletAccountType {
  id: string
  name: string
  address: string
}

export interface Props {
  siteURL: string
  accounts: WalletAccountType[]
  onSubmit: () => void
  onCancel: () => void
  actionButtonText: string
  selectAccount: (account: WalletAccountType) => void
  removeAccount: (account: WalletAccountType) => void
  selectedAccounts: WalletAccountType[]
}

export default class ConnectWithSite extends React.PureComponent<Props> {
  checkIsSelected = (account: WalletAccountType) => {
    return this.props.selectedAccounts.some((a) => a.id === account.id)
  }

  toggleSelected = (account: WalletAccountType) => () => {
    if (this.checkIsSelected(account)) {
      this.props.removeAccount(account)
    } else {
      this.props.selectAccount(account)
    }
  }

  render () {
    const {
      onSubmit,
      onCancel,
      siteURL,
      accounts,
      actionButtonText
    } = this.props
    return (
      <StyledWrapper>
        <ConnectHeader detailText={siteURL} />
        <SelectAddressContainer>
          <NewAccountTitle>New Account</NewAccountTitle>
          <DividerLine />
          <SelectAddressScrollContainer>
            {accounts.map((account) => (
              <SelectAddress
                action={this.toggleSelected(account)}
                key={account.id}
                account={account}
                isSelected={this.checkIsSelected(account)}
              />
            ))}
          </SelectAddressScrollContainer>
          <DividerLine />
        </SelectAddressContainer>
        <ConnectBottomNav
          actionText={actionButtonText}
          onSubmit={onSubmit}
          onCancel={onCancel}
        />
      </StyledWrapper>
    )
  }
}
