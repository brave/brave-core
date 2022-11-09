import * as React from 'react'
import { StyledWrapper, StyledContent } from './style'

export interface Props {
  maintainWidth?: boolean
  children?: React.ReactNode
}

export default class WalletPageLayout extends React.PureComponent<Props, {}> {
  render () {
    const { children, maintainWidth } = this.props
    return (
      <StyledWrapper maintainWidth={maintainWidth}>
        <StyledContent maintainWidth={maintainWidth}>
          {children}
        </StyledContent>
      </StyledWrapper>
    )
  }
}
