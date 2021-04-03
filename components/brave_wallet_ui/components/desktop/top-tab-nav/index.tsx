import * as React from 'react'
import { TopTabNavObjectType, TopTabNavTypes } from '../../../constants/types'
// Styled Components
import { StyledWrapper } from './style'

// Components
import { TopTabNavButton } from '../'

export interface Props {
  tabList: TopTabNavObjectType[]
  selectedTab: TopTabNavTypes
  onSubmit: (id: TopTabNavTypes) => void
}

export default class TopTabNav extends React.PureComponent<Props, {}> {

  onNav = (id: TopTabNavTypes) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { tabList, selectedTab } = this.props
    return (
      <StyledWrapper>
        {tabList.map((option) =>
          <TopTabNavButton
            key={option.id}
            isSelected={selectedTab === option.id}
            onSubmit={this.onNav(option.id)}
            text={option.name}
          />
        )}
      </StyledWrapper>
    )
  }
}
