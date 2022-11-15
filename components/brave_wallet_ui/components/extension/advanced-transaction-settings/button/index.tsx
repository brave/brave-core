// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import { StyledButton, SettingsIcon, TabLine } from './style'

export interface Props {
  onSubmit?: () => void
}

const AdvancedTransactionSettingsButton = (props: Props) => {
  const { onSubmit } = props
  return (
    <StyledButton onClick={onSubmit}>
      <SettingsIcon />
      <TabLine />
    </StyledButton>
  )
}

export default AdvancedTransactionSettingsButton
