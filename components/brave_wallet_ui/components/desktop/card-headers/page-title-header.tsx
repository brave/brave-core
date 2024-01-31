// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Hooks
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Components
import { DefaultPanelHeader } from './default-panel-header'

// styles
import { Row } from '../../shared/style'
import {
  ButtonIcon,
  CircleButton,
  HeaderTitle
} from './shared-card-headers.style'

interface Props {
  title: string
  onBack?: () => void
}

export const PageTitleHeader = ({ title, onBack }: Props) => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // render
  if (isPanel && !onBack) {
    return <DefaultPanelHeader title={title} />
  }

  return (
    <Row
      padding={isPanel ? '17px 20px' : '24px 0px'}
      justifyContent='flex-start'
    >
      {onBack && (
        <CircleButton
          size={28}
          marginRight={16}
          onClick={onBack}
        >
          <ButtonIcon
            size={16}
            name='arrow-left'
          />
        </CircleButton>
      )}
      <HeaderTitle isPanel={isPanel}>{title}</HeaderTitle>
    </Row>
  )
}
