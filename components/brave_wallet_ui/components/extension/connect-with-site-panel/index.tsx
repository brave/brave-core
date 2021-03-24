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
  SelectAddressScrollContainer,
  Details,
  MiddleWrapper,
  AccountListWrapper,
  ConfirmTextRow,
  ConfirmTextColumn,
  ConfirmText,
  ConfirmIcon
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { WalletAccountType } from '../../../constants/types'

export interface Props {
  siteURL: string
  accounts: WalletAccountType[]
  primaryAction: () => void
  secondaryAction: () => void
  selectAccount: (account: WalletAccountType) => void
  removeAccount: (account: WalletAccountType) => void
  selectedAccounts: WalletAccountType[]
  isReady: boolean
}

export default class ConnectWithSite extends React.PureComponent<Props> {
  checkIsSelected = (account: WalletAccountType) => {
    return this.props.selectedAccounts.some((a) => a.id === account.id)
  }

  createAccountList = () => {
    const list = this.props.selectedAccounts.map((a) => {
      return reduceAddress(a.address)
    })
    return list.join(', ')
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
      primaryAction,
      secondaryAction,
      siteURL,
      accounts,
      isReady,
      selectedAccounts
    } = this.props
    return (
      <StyledWrapper>
        <ConnectHeader url={siteURL} />
        <MiddleWrapper>
          {isReady ? (
            <AccountListWrapper>
              <Details>{this.createAccountList()}</Details>
            </AccountListWrapper>
          ) : (
            <Details>Select accounts(s)</Details>
          )}
          {!isReady ? (
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
          ) : (
            <ConfirmTextRow>
              <ConfirmIcon />
              <ConfirmTextColumn>
                <ConfirmText>View the addressess of your</ConfirmText>
                <ConfirmText>permitted accounts (required)</ConfirmText>
              </ConfirmTextColumn>
            </ConfirmTextRow>
          )}
        </MiddleWrapper>
        <ConnectBottomNav
          primaryText={isReady ? 'Connect' : 'Next'}
          secondaryText={isReady ? 'Back' : 'Cancel'}
          primaryAction={primaryAction}
          secondaryAction={secondaryAction}
          disabled={selectedAccounts.length === 0}
        />
      </StyledWrapper>
    )
  }
}
