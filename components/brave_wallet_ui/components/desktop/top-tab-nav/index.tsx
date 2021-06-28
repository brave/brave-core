import * as React from 'react'
import { TopTabNavObjectType, TopTabNavTypes, AddAccountNavTypes } from '../../../constants/types'
// Styled Components
import {
  StyledWrapper,
  LockIcon,
  MoreRow,
  MoreButton,
  EmptyPadding,
  Line
} from './style'

// Components
import { TopTabNavButton } from '../'

export interface Props {
  tabList: TopTabNavObjectType[]
  selectedTab: TopTabNavTypes | AddAccountNavTypes
  onSubmit: (id: TopTabNavTypes | AddAccountNavTypes) => void
  onLockWallet?: () => void
  hasMoreButtons?: boolean
  onClickMoreButton?: () => void
}

export default class TopTabNav extends React.PureComponent<Props, {}> {
  onNav = (id: TopTabNavTypes | AddAccountNavTypes) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { tabList, selectedTab, hasMoreButtons, onLockWallet } = this.props
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

        <MoreRow>
          {hasMoreButtons ? (
            <MoreButton onClick={onLockWallet}>
              <LockIcon />
            </MoreButton>
          ) : (
            <EmptyPadding />
          )}
          <Line />
        </MoreRow>
      </StyledWrapper>
    )
  }
}
