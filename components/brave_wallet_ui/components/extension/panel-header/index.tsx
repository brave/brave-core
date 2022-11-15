// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { SearchBar } from '../../shared'

// Styled Components
import {
  HeaderTitle,
  HeaderWrapper,
  TopRow,
  CloseButton
} from './style'
import { getLocale } from '../../../../common/locale'
import { PanelTypes } from '../../../constants/types'

export interface Props {
  title: string
  action: (path: PanelTypes) => void
  useSearch?: boolean | undefined
  searchAction?: (event: any) => void | undefined
}

export default class PanelHeader extends React.PureComponent<Props> {
  navigate = (path: PanelTypes) => () => {
    this.props.action(path)
  }

  render () {
    const { title, searchAction, useSearch } = this.props
    return (
      <HeaderWrapper hasSearch={useSearch || false}>
        <TopRow>
          <HeaderTitle>{title}</HeaderTitle>
          <CloseButton onClick={this.navigate('main')} />
        </TopRow>
        {useSearch &&
          <SearchBar
            placeholder={getLocale('braveWalletSearchText')}
            action={searchAction}
          />
        }
      </HeaderWrapper>
    )
  }
}
