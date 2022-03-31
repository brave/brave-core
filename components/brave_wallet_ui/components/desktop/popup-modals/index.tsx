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

const ESC_KEY = 'Escape'

const PopupModal = (props: Props) => {
  const { title, onClose, children } = props

  const handleKeyDown = (event: KeyboardEvent) => {
    if (event.key === ESC_KEY) {
      onClose()
    }
  }

  React.useEffect(() => {
    document.addEventListener('keydown', handleKeyDown)

    return () => {
      document.removeEventListener('keydown', handleKeyDown)
    }
  }, [])

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
