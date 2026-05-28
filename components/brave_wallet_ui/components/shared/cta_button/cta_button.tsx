// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { openTab } from '../../../utils/routes-utils'

// Styled Components
import { Button, ButtonText, IdeaButtonIcon } from './cta_button.style'

interface Props {
  buttonText: string
  url: string
}

export const CTAButton = ({ buttonText, url }: Props) => {
  const onClick = React.useCallback(() => {
    openTab(url)
  }, [url])

  return (
    <Button onClick={onClick}>
      <IdeaButtonIcon />
      <ButtonText variant='default.semibold'>{buttonText}</ButtonText>
    </Button>
  )
}
