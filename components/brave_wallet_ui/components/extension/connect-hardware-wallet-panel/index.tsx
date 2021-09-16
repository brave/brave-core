import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  InstructionsButton,
  ButtonWrapper,
  Indicator,
  ConnectionRow
} from './style'
import { NavButton } from '..'
import locale from '../../../constants/locale'

export interface Props {
  onCancel: () => void
  isConnected: boolean
  walletName: string
}

function ConnectHardwareWalletPanel (props: Props) {
  const { onCancel, walletName, isConnected } = props

  const onClickInstructions = () => {
    window.open('https://support.brave.com/hc/en-us/articles/4409309138701', '_blank')
  }

  return (
    <StyledWrapper>
      <ConnectionRow>
        <Indicator isConnected={isConnected} />
        <Description>{walletName} {isConnected ? locale.connectHardwarePanelConnected : locale.connectHardwarePanelDisconnected}</Description>
      </ConnectionRow>
      <Title>{locale.connectHardwarePanelConnect} {walletName}</Title>
      <InstructionsButton onClick={onClickInstructions}>{locale.connectHardwarePanelInstructions}</InstructionsButton>
      <PageIcon />
      <ButtonWrapper>
        <NavButton buttonType='secondary' text={locale.backupButtonCancel} onSubmit={onCancel} />
      </ButtonWrapper>
    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
