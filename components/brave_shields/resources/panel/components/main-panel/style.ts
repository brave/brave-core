import styled, { css } from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
interface CaratIconProps {
  isExpanded: boolean
}

export const Box = styled.div`
  background-color: ${(p) => p.theme.color.background02};
  width: 100%;
  height: 100%;

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    color: ${(p) => p.theme.color.interactive05};
    text-decoration: underline;
  }
`

export const PanelHeader = styled.section`
  width: 100%;
  padding: 24px 22px 24px 22px;
  box-sizing: border-box;
`

export const ToggleBox = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  margin-bottom: 15px;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 0 0 25px 0;
  position: relative;
  z-index: 2;
`

export const SiteTitle = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 22px;
  font-weight: 500;
  letter-spacing: 0.02em;
  margin: 0;
  text-align: center;
`

export const AdvancedControlsButton = styled.button`
  --border: 3px solid transparent;
  --text-color: ${(p) => p.theme.color.text01};

  background-color: ${(p) => p.theme.color.background03};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  color: var(--text-color);
  width: 100%;
  padding: 14px 22px;
  border: var(--border);
  display: flex;
  align-items: center;
  justify-content: space-between;

  &:hover {
    --text-color: ${(p) => p.theme.color.interactive05};
  }

  &:focus {
    --border: 3px solid ${(p) => p.theme.color.focusBorder};
  }
`

export const CaratIcon = styled(CaratStrongDownIcon)<CaratIconProps>`
  --rotate: rotate(0deg);

  width: 24px;
  height: 24px;
  transform: var(--rotate);

  ${p => p.isExpanded && css`
    --rotate: rotate(180deg);
  `}
`

export const StatusText = styled.div`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 16px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  margin: 22px 0;

  span {
    font-weight: 600;
  }
`

export const BlockCountBox = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 20px;
  max-width: 215px;
  margin: 0 auto 16px auto;
`

export const BlockCount = styled.span`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 38px;
  font-weight: 400;
  color: ${(p) => p.theme.color.text01};
  pointer-events: none;
`

export const BlockNote = styled.span`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
`

export const Footnote = styled.div`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
  color: ${(p) => p.theme.color.text03};
  text-align: center;
  padding: 0 26px;
`
