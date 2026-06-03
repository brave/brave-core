// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Utils
import { openTab } from '../../../utils/routes-utils'

// Styled Components
import { ButtonWrapper } from './cta_button.style'

interface Props {
  buttonText: string
  url: string
  iconName?: string
}

export const CTAButton = ({ buttonText, url, iconName }: Props) => {
  const onClick = React.useCallback(() => {
    openTab(url)
  }, [url])

  return (
    <ButtonWrapper width='unset'>
      <Button onClick={onClick}>
        {iconName && (
          <Icon
            slot='icon-before'
            name={iconName}
          />
        )}
        {buttonText}
      </Button>
    </ButtonWrapper>
  )
}
