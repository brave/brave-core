import * as React from 'react'

// Styled Components
import {
  HeaderTitle,
  HeaderWrapper,
  ActionIcon,
  ExpandIcon
} from './style'

export interface Props {
  action: (path: string) => void
}

export default class ConnectedHeader extends React.PureComponent<Props> {

  navigate = (path: string) => () => {
    this.props.action(path)
  }

  render () {
    return (
      <HeaderWrapper>
        <ExpandIcon onClick={this.navigate('expanded')} />
        <HeaderTitle>Brave Web 3 Connect</HeaderTitle>
        <ActionIcon onClick={this.navigate('settings')} />
      </HeaderWrapper>
    )
  }
}
