// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { WalletRoutes } from '../../../constants/types'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Hooks
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Components
import { DefaultPanelHeader } from './default-panel-header'

// styles
import { Row } from '../../shared/style'
import {
  MenuButtonIcon,
  MenuButton,
  HeaderTitle,
} from './shared-card-headers.style'

interface Props {
  title: string
  showBackButton?: boolean
  onBack?: () => void
  expandRoute?: WalletRoutes
}

export const PageTitleHeader = ({
  title,
  showBackButton,
  onBack,
  expandRoute,
}: Props) => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)
  const isAndroidOrPanel = isAndroid || isPanel

  return isAndroidOrPanel && !showBackButton ? (
    <DefaultPanelHeader
      title={title}
      expandRoute={expandRoute}
    />
  ) : (
    <Row
      padding={isAndroidOrPanel ? '17px 20px' : '24px 0px'}
      justifyContent='flex-start'
    >
      {showBackButton && (
        <MenuButton
          marginRight={16}
          onClick={onBack}
        >
          <MenuButtonIcon
            size={16}
            name='arrow-left'
          />
        </MenuButton>
      )}
      <HeaderTitle isAndroidOrPanel={isAndroidOrPanel}>{title}</HeaderTitle>
    </Row>
  )
}
