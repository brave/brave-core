import styled from 'styled-components'
import CloseIcon from '../assets/close.svg'

interface StyleProps {
  hasSearch: boolean
}

export const HeaderTitle = styled.span`
  font-family: Poppins;
  font-size: 18px;
  line-height: 26px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  text-transform: capitalize;
`

export const HeaderWrapper = styled.div<StyleProps>`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border-bottom: ${(p) => `1px solid ${p.theme.color.divider01}`};
  padding: 0px 12px;
  max-width: 300px;
  margin-bottom: ${(p) => p.hasSearch ? '0px' : '8px'};
`

export const TopRow = styled.div`
  display: flex;
  height: 54px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
`

export const CloseButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background: url(${CloseIcon});
  outline: none;
  border: none;
`
