// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
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
