import * as React from 'react'

// Components
import { PanelHeader } from '../'

// Styled Components
import {
  StyledWrapper
} from './style'

// Utils
import { PanelTypes } from '../../../constants/types'

export interface Props {
  title: string
  navAction: (path: PanelTypes) => void
}

export default class Panel extends React.PureComponent<Props> {
  render () {
    const { title, navAction, children } = this.props
    return (
      <StyledWrapper>
        <PanelHeader
          action={navAction}
          title={title}
        />
        {children}
      </StyledWrapper>
    )
  }
}
