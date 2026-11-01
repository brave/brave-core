// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css, keyframes } from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'

const slideIn = keyframes`
  from {
    opacity: 0;
    transform: translateY(${leo.spacing['2Xl']});
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
`

const Box = styled.div<{ animateIn?: boolean }>`
  position: fixed;
  right: ${leo.spacing['2Xl']};
  bottom: ${leo.spacing['2Xl']};

  ${p => p.animateIn && css`
    animation: ${slideIn} ${leo.duration.m} ${leo.easing.inOut} both;
  `}

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }

  leo-button {
    --leo-button-color: var(--window-surface);
  }

  leo-icon {
    --leo-icon-color: var(--window-accent);
    --leo-icon-size: var(--leo-icon-m);
  }
`

interface Props {
  onClick: () => void
  // Slides in when it replaces cards the user just closed; on a page that
  // opens with the cards already hidden it's simply there.
  animateIn?: boolean
}

function InfoButton (props: Props) {
  return (
    <Box animateIn={props.animateIn}>
      <Button
        fab
        kind='filled'
        size='small'
        onClick={props.onClick}
        title={getLocale('showInfoLabel')}
        aria-label={getLocale('showInfoLabel')}
        data-testid='info-cards-show-button'
      >
        <Icon name='info-outline' />
      </Button>
    </Box>
  )
}

export default InfoButton
