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
  title: string
}

const PopupModal = (props: Props) => {
  const { title, onClose, children } = props

  return (
    <StyledWrapper>
      <Modal>
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
