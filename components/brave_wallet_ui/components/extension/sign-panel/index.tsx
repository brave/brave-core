import * as React from 'react'
import { create } from 'ethereum-blockies'
import { WalletAccountType, EthereumChain } from '../../../constants/types'
import locale from '../../../constants/locale'
import { NavButton } from '../'
// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  MessageTitle,
  MessageText,
  ButtonRow,
  MoreButton
} from './style'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: EthereumChain
  message: string
  onSign: () => void
  onCancel: () => void
  onClickMore: () => void
}

function SignPanel (props: Props) {
  const {
    selectedAccount,
    selectedNetwork,
    message,
    onSign,
    onCancel,
    onClickMore
  } = props

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{selectedNetwork.chainName}</NetworkText>
        <MoreButton onClick={onClickMore} />
      </TopRow>
      <AccountCircle orb={orb} />
      <AccountNameText>{selectedAccount.name}</AccountNameText>
      <PanelTitle>{locale.signTransactionTitle}</PanelTitle>
      <MessageBox>
        <MessageTitle>{locale.signTransactionMessageTitle}</MessageTitle>
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
