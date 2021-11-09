import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ClickAwayContainer
} from './style'

interface TransactionPopupItemProps {
  onClick: () => void
  text: string
}

export const TransactionPopupItem = (props: TransactionPopupItemProps) => (
  <PopupButton onClick={props.onClick}>
    <PopupButtonText>{props.text}</PopupButtonText>
  </PopupButton>
)

interface Props {
  children?: React.ReactNode
}

const TransactionPopup = (props: Props) => {
  return (
    <>
      <StyledWrapper>
        {props.children}
      </StyledWrapper>
      <ClickAwayContainer />
    </>
  )
}

export default TransactionPopup
