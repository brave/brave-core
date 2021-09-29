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
      <HeaderWrapper hasSearch={useSearch ? useSearch : false}>
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
