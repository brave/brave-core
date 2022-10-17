import * as React from 'react'
import { StyledWrapper } from './style'

export interface Props {
  noPadding?: boolean
  children?: React.ReactNode
}

export default class WalletSubViewLayout extends React.PureComponent<Props, {}> {
  render () {
    const { children, noPadding } = this.props
    return (
      <StyledWrapper noPadding={noPadding}>
        {children}
      </StyledWrapper>
    )
  }
}
