import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  Header,
  Title,
  CloseButton,
  Modal
} from './style'

export interface Props {
  children?: React.ReactNode
  onClose: () => void
  title?: string
  isFlexible?: boolean
}

const PopupModal = (props: Props) => {
  const { title, onClose, children, isFlexible } = props

  return (
    <StyledWrapper>
      <Modal isFlexible={isFlexible ?? false}>
        <Header>
          <Title>{title}</Title>
          <CloseButton onClick={onClose} />
        </Header>
        {children}
      </Modal>
    </StyledWrapper>
  )
}

export default PopupModal
