import * as React from 'react'

// Styled Components
import {
  HeaderTitle,
  HeaderWrapper,
  CloseButton
} from './style'

import { PanelTypes } from '../../../constants/types'

export interface Props {
  title: string
  action: (path: PanelTypes) => void
}

export default class PanelHeader extends React.PureComponent<Props> {

  navigate = (path: PanelTypes) => () => {
    this.props.action(path)
  }

  render () {
    const { title } = this.props
    return (
      <HeaderWrapper>
        <HeaderTitle>{title}</HeaderTitle>
        <CloseButton onClick={this.navigate('main')} />
      </HeaderWrapper>
    )
  }
}
