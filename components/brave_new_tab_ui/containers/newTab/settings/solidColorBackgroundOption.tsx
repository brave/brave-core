import * as React from 'react'

import {
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionSolidColor,
  StyledCustomBackgroundOptionLabel,
} from '../../../components/default'

interface Props {
  color: string
  name: string
  useSolidColorBackground: (color: string) => void
}

class SolidColorBackgroundOption extends React.PureComponent<Props, {}> {
  onClickColor = () => {
    this.props.useSolidColorBackground(this.props.color);
  }

  render() {
    const { color } = this.props

    return (<StyledCustomBackgroundOption onClick={this.onClickColor}>
              <StyledCustomBackgroundOptionSolidColor style={{ backgroundColor: color }} />
              <StyledCustomBackgroundOptionLabel>
                {this.props.name}
              </StyledCustomBackgroundOptionLabel>
            </StyledCustomBackgroundOption>)
  }
}

export default SolidColorBackgroundOption
