import * as React from 'react'
import { TopTabNavObjectType, TopTabNavTypes } from '../../../constants/types'
// Styled Components
import {
  StyledWrapper,
  MoreRow,
  MoreButton,
  MoreIcon,
  Line
} from './style'

// Components
import { TopTabNavButton } from '../'

export interface Props {
  tabList: TopTabNavObjectType[]
  selectedTab: TopTabNavTypes
  onSubmit: (id: TopTabNavTypes) => void
  hasMoreButton?: boolean
  onClickMoreButton?: () => void
}

export default class TopTabNav extends React.PureComponent<Props, {}> {
  onNav = (id: TopTabNavTypes) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { tabList, selectedTab, hasMoreButton, onClickMoreButton } = this.props
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
        {hasMoreButton &&
          <MoreRow>
            <MoreButton onClick={onClickMoreButton}>
              <MoreIcon />
            </MoreButton>
            <Line />
          </MoreRow>
        }
      </StyledWrapper>
    )
  }
}
