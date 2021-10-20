import * as React from 'react'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ClickAwayContainer
} from './style'

export interface Props {
  onClickView: () => void
}

const TransactionPopup = (props: Props) => {
  const {
    onClickView
  } = props

  return (
    <>
      <StyledWrapper>
        <PopupButton onClick={onClickView}>
          <PopupButtonText>{getLocale('braveWalletTransactionExplorer')}</PopupButtonText>
        </PopupButton>
      </StyledWrapper>
      <ClickAwayContainer />
    </>
  )
}

export default TransactionPopup
