// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  ModalButton,
  ModalButtonIcon
} from './account-modal.style'

interface Props {
  iconName: string
  text: string
  onClick: () => void
}

export const AccountModalButton = (props: Props) => {
  const { onClick, iconName, text } = props

  return (
    <ModalButton onClick={onClick}>
      <ModalButtonIcon size={20} name={iconName} />
      {text}
    </ModalButton>
  )
}
