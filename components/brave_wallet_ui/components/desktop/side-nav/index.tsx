import * as React from 'react'
import { NavObjectType, NavTypes } from '../../../constants/types'
// Styled Components
import { StyledWrapper } from './style'

// Components
import { SideNavButton } from '../'

export interface Props {
  navList: NavObjectType[]
  selectedButton: NavTypes
  onSubmit: (id: NavTypes) => void
}

export default class SideNav extends React.PureComponent<Props, {}> {

  onNav = (id: NavTypes) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { navList, selectedButton } = this.props
    return (
      <StyledWrapper>
        {navList.map((option) =>
          <SideNavButton
            key={option.id}
            isSelected={selectedButton === option.id}
            onSubmit={this.onNav(option.id)}
            text={option.name}
            icon={selectedButton === option.id ? option.primaryIcon : option.secondaryIcon}
          />
        )}
      </StyledWrapper>
    )
  }
}
