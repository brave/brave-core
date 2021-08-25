import * as React from 'react'

// Styled Components
import {
  StyledButton,
  ButtonText,
  RejectIcon,
  SignIcon,
  ConfirmIcon
} from './style'

export type PanelButtonTypes =
  | 'primary'
  | 'secondary'
  | 'danger'
  | 'confirm'
  | 'sign'
  | 'reject'

export interface Props {
  buttonType: PanelButtonTypes
  text: string | undefined
  onSubmit: () => void
  disabled?: boolean
}

export default class NavButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, buttonType, disabled } = this.props
    return (
      <StyledButton disabled={disabled} buttonType={buttonType} onClick={onSubmit}>
        {buttonType === 'reject' &&
          <RejectIcon />
        }
        {buttonType === 'sign' &&
          <SignIcon />
        }
        {buttonType === 'confirm' &&
          <ConfirmIcon />
        }
        <ButtonText buttonType={buttonType}>{text}</ButtonText>
      </StyledButton>
    )
  }
}
