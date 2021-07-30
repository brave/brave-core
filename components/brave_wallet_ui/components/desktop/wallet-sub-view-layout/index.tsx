import * as React from 'react'
import { StyledWrapper } from './style'

export interface Props {
  children?: React.ReactNode
}

export default class WalletSubViewLayout extends React.PureComponent<Props, {}> {
  render () {
    const { children } = this.props
    return (
      <StyledWrapper>
        {children}
      </StyledWrapper>
    )
  }
}
