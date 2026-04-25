// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Button, ButtonIcon } from './toggle-theme-button.style'

interface Props {
  onClick: () => void
}

export const ToggleThemeButton = (props: Props) => {
  const { onClick } = props

  return (
    <Button onClick={onClick}>
      <ButtonIcon />
    </Button>
  )
}

export default ToggleThemeButton
