import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText, PlusIcon, EditIcon } from './style'

export interface Props {
  buttonType: 'primary' | 'secondary'
  text: string | undefined
  onSubmit: () => void
  disabled?: boolean
  editIcon?: boolean
}

export default class AddButton extends React.PureComponent<Props, {}> {
  render () {
    const {
      onSubmit,
      text,
      buttonType,
      disabled,
      editIcon
    } = this.props
    return (
      <StyledButton disabled={disabled} buttonType={buttonType} onClick={onSubmit}>
        {!editIcon ? (
          <PlusIcon />
        ) : (
          <EditIcon />
        )}
        <ButtonText buttonType={buttonType}>{text}</ButtonText>
      </StyledButton>
    )
  }
}
