import * as React from 'react'

import {
  PopupModal
} from '../..'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  Description,
  Title,
  ModalIcon
} from './style'

export interface Props {
  onClose: () => void
  onAddAccount: () => void
}

const WelcomeModal = (props: Props) => {
  const { onClose, onAddAccount } = props

  return (
    <PopupModal onClose={onClose} isFlexible={true}>
      <StyledWrapper>
        <ModalIcon />
        <Title>{locale.welcomeModalTitle}</Title>
        <Description>{locale.welcomeModalDescription}</Description>
        <NavButton
          onSubmit={onAddAccount}
          text={locale.addAccount}
          buttonType='primary'
        />
      </StyledWrapper>
    </PopupModal>
  )
}

export default WelcomeModal
