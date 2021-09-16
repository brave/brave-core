import * as React from 'react'
import { create } from 'ethereum-blockies'
import { WalletAccountType, EthereumChain } from '../../../constants/types'
import locale from '../../../constants/locale'
import { NavButton, PanelTab } from '../'
// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  MessageText,
  ButtonRow
} from './style'
import { TabRow } from '../shared-panel-styles'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: EthereumChain
  message: string
  onSign: () => void
  onCancel: () => void
}

function SignPanel (props: Props) {
  const {
    selectedAccount,
    selectedNetwork,
    message,
    onSign,
    onCancel
  } = props

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{selectedNetwork.chainName}</NetworkText>
      </TopRow>
      <AccountCircle orb={orb} />
      <AccountNameText>{selectedAccount.name}</AccountNameText>
      <PanelTitle>{locale.signTransactionTitle}</PanelTitle>
      <TabRow>
        <PanelTab
          isSelected={true}
          text={locale.signTransactionMessageTitle}
        />
      </TabRow>
      <MessageBox>
        <MessageText>{message}</MessageText>
      </MessageBox>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={locale.backupButtonCancel}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='sign'
          text={locale.signTransactionButton}
          onSubmit={onSign}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default SignPanel
