// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { WalletRoutes } from '../../../constants/types'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Components
import { DefaultPanelHeader } from './default-panel-header'
import { SearchBar } from '../../shared/search_bar/search_bar'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Styled Components
import { SearchBarWrapper } from './activity_page_header.style'
import { HeaderTitle } from './shared-card-headers.style'
import { Row } from '../../shared/style'

export interface Props {
  searchValue: string
  onSearchValueChange: (value: string) => void
}

export const ActivityPageHeader = (props: Props) => {
  const { searchValue, onSearchValueChange } = props

  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return isPanel ? (
    <DefaultPanelHeader
      title={getLocale('braveWalletActivity')}
      expandRoute={WalletRoutes.Activity}
    />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle>{getLocale('braveWalletActivity')}</HeaderTitle>
      <SearchBarWrapper alignItems='flex-start'>
        <SearchBar
          placeholder={getLocale('braveWalletSearchText')}
          onChange={onSearchValueChange}
          value={searchValue}
        />
      </SearchBarWrapper>
    </Row>
  )
}

export default ActivityPageHeader
