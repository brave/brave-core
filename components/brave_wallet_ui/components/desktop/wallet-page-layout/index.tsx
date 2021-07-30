import * as React from 'react'
import { StyledWrapper, StyledContent } from './style'

export interface Props {
  children?: React.ReactNode
}

export default class WalletPageLayout extends React.PureComponent<Props, {}> {
  render () {
    const { children } = this.props
    return (
      <StyledWrapper>
        <StyledContent>
          {children}
        </StyledContent>
      </StyledWrapper>
    )
  }
}
