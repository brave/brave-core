import styled from 'styled-components'
import { BuySendSwapTypes } from '../../../constants/types'

interface StyleProps {
  isSelected: boolean
  selectedTab: BuySendSwapTypes
  tabID: BuySendSwapTypes
}

export const StyledWrapper = styled.div`
  position: relative;
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding-top: 36px;
`

export const MainContainerWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 0px 0px 8px 8px;
`

export const MainContainer = styled.div<Partial<StyleProps>>`
  position: relative;
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 18px;
  background-color: ${(p) => p.theme.color.background02};
  border: ${(p) => `2px solid ${p.theme.color.divider01}`};
  border-radius: ${(p) =>
    p.selectedTab === 'buy' ? '0px 8px 8px 8px' : p.selectedTab === 'swap' ? '8px 0px 8px 8px' : '8px'};
  // min-height: 220px;
  max-height: 470px;
  overflow: hidden;
`

export const ButtonRow = styled.div`
  display: flex;
  height: 38px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  position: absolute;
  top: 0px;
`

export const TabButton = styled.button<Partial<StyleProps>>`
  flex: 1;
  display: flex;
  height: 100%;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: ${(p) =>
    p.isSelected ? p.theme.color.background02 : p.theme.color.background01};
  border-radius: 12px 12px 0px 0px;
  border: ${(p) =>
    p.isSelected ? `2px solid ${p.theme.color.divider01}` : `2px solid ${p.theme.color.background01}`};
  border-bottom-width: 0px;
  z-index: ${(p) =>
    p.isSelected ? '2' : '0'};
  position: relative;
`

export const TabButtonText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  background: ${(p) =>
    p.isSelected ? p.theme.color.text01 : p.theme.color.text02};
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
`

export const RightDivider = styled.div<Partial<StyleProps>>`
  height: 20px;
  width: 1px;
  background-color: ${(p) => p.selectedTab === 'buy' && p.tabID === 'send' ? p.theme.color.divider01 : p.selectedTab === 'swap' && p.tabID === 'buy' ? p.theme.color.divider01 : 'none'};
  position: absolute;
  right: -2px;
  bottom: 4px;
`

export const LeftDivider = styled.div<Partial<StyleProps>>`
  height: 20px;
  width: 1px;
  background-color: ${(p) => p.selectedTab === 'buy' && p.tabID === 'swap' ? p.theme.color.divider01 : p.selectedTab === 'swap' && p.tabID === 'send' ? p.theme.color.divider01 : 'none'};
  position: absolute;
  left: -2px;
  bottom: 4px;
`
