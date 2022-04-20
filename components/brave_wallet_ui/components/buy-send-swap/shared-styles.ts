import styled from 'styled-components'
import { WalletButton } from '../shared/style'

export const BubbleContainer = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  border-radius: 12px;
  padding: 5px 12px;
  background-color: ${(p) => p.theme.color.background02};
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  margin-bottom: 12px;
  box-sizing: border-box;
`

export const SelectWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
`

export const SelectScrollSearchContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  overflow-y: scroll;
  overflow-x: hidden;
  position: absolute;
  top: 96px;
  bottom: 18px;
  left: 18px;
  right: 18px;
`

export const SelectScrollContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  overflow-y: scroll;
  overflow-x: hidden;
  position: absolute;
  top: 50px;
  bottom: 18px;
  left: 18px;
  right: 18px;
`

export const ErrorText = styled.span`
  font-family: Poppins;
  letter-spacing: 0.01em;
  font-size: 12px;
  color: ${(p) => p.theme.color.errorText};
  word-break: break-word;
  margin-bottom: 12px;
`

export const DivderTextWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  border-bottom: 2px solid ${(p) => p.theme.color.divider01};
  padding-bottom: 6px;
  margin-bottom: 10px;
`

export const DividerText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const ResetButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  font-weight: 600;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  padding: 2px 0px;
  width: 48px;
  margin: 12px 0px;
  background: none;
  color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.palette.blurple300};
  }
`
