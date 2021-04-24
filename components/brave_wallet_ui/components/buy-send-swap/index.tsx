import * as React from 'react'
import { BuySendSwapTypes } from '../../constants/types'
import { BuySendSwapOptions } from '../../options/buy-send-swap-options'
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

const BuySendSwap = () => {
  const [selectedTab, setSelectedTab] = React.useState<BuySendSwapTypes>('buy')
  const changeTab = (tab: BuySendSwapTypes) => () => {
    setSelectedTab(tab)
  }

  return (
    <StyledWrapper>
      <ButtonRow>
        {BuySendSwapOptions.map((option) =>
          <TabButton key={option.id} isSelected={selectedTab === option.id} onClick={changeTab(option.id)}>
            <RightDivider tabID={option.id} selectedTab={selectedTab} />
            <LeftDivider tabID={option.id} selectedTab={selectedTab} />
            <TabButtonText>{option.name}</TabButtonText>
          </TabButton>
        )}
      </ButtonRow>
      <MainContainerWrapper>
        <MainContainer selectedTab={selectedTab}>
          <span>{selectedTab.toUpperCase()} Container</span>
        </MainContainer>
      </MainContainerWrapper>
    </StyledWrapper>
  )
}

export default BuySendSwap
