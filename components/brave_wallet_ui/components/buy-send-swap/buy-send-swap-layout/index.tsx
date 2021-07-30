import * as React from 'react'
import { BuySendSwapTypes } from '../../../constants/types'
import { BuySendSwapOptions } from '../../../options/buy-send-swap-options'
// Styled Components
import {
  StyledWrapper,
  MainContainer,
  MainContainerWrapper,
  ButtonRow,
  TabButton,
  TabButtonText,
  RightDivider,
  LeftDivider
} from './style'

export interface Props {
  children?: React.ReactNode
  selectedTab: BuySendSwapTypes
  onChangeTab: (tab: BuySendSwapTypes) => () => void
}

function BuySendSwapLayout (props: Props) {
  const { children, selectedTab, onChangeTab } = props

  return (
    <StyledWrapper>
      <ButtonRow>
        {BuySendSwapOptions.map((option) =>
          <TabButton
            key={option.id}
            isSelected={selectedTab === option.id}
            onClick={onChangeTab(option.id)}
          >
            <RightDivider
              tabID={option.id}
              selectedTab={selectedTab}
            />
            <LeftDivider
              tabID={option.id}
              selectedTab={selectedTab}
            />
            <TabButtonText
              isSelected={selectedTab === option.id}
            >
              {option.name}
            </TabButtonText>
          </TabButton>
        )}
      </ButtonRow>
      <MainContainerWrapper>
        <MainContainer selectedTab={selectedTab}>
          {children}
        </MainContainer>
      </MainContainerWrapper>
    </StyledWrapper>
  )
}

export default BuySendSwapLayout
