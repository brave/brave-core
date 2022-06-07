import * as React from 'react'

// Components
import { PanelHeader, PanelHeaderSlim } from '../'

// Styled Components
import {
  StyledWrapper
} from './style'

// Utils
import { PanelTypes, PanelHeaderSizes } from '../../../constants/types'

export interface Props {
  title: string
  headerStyle?: PanelHeaderSizes
  navAction: (path: PanelTypes) => void
  useSearch?: boolean | undefined
  searchAction?: (event: any) => void | undefined
}

export default class Panel extends React.PureComponent<Props> {
  render () {
    const {
      title,
      headerStyle,
      navAction,
      children,
      searchAction,
      useSearch
    } = this.props

    return (
      <StyledWrapper>
        {headerStyle === 'slim'
          ? <PanelHeaderSlim
              action={navAction}
              title={title}
            />
          : <PanelHeader
              action={navAction}
              title={title}
              searchAction={searchAction}
              useSearch={useSearch}
            />
        }
        {children}
      </StyledWrapper>
    )
  }
}
