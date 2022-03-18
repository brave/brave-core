import styled, { css } from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import globalIconUrl from '../../../../../web-components/icons/globe.svg'

interface CaratIconProps {
  isExpanded: boolean
}

interface ShieldsIconProps {
  isActive: boolean
}

export const Box = styled.div`
  background-color: ${(p) => p.theme.color.background02};
  width: 100%;
  height: 100%;
  font-family: ${(p) => p.theme.fontFamily.heading};

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    color: ${(p) => p.theme.color.interactive05};
    text-decoration: underline;
  }
`

export const HeaderBox = styled.section`
  background-color: ${(p) => p.theme.color.background01};
  padding: 22px 17px 22px 17px;
`

export const SiteTitleBox = styled.section`
  width: 100%;
  display: grid;
  grid-template-columns: 24px 1fr 10px;
  grid-gap: 10px;
  align-items: center;
  margin-bottom: 10px;
`

export const FavIconBox = styled.i`
  width: 22px;
  height: 22px;

  img {
    width: 100%;
    height: auto;
    object-fit: contain;
  }
`

export const SiteTitle = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-size: 20px;
  line-height: 1.4;
  font-weight: 500;
  letter-spacing: 0.02em;
  margin: 0;
  overflow: hidden;
  text-align: left;
  // We truncate long site titles to the left
  direction: rtl;
  text-overflow: ellipsis;
  white-space: nowrap;
`

export const CountBox = styled.section`
  display: grid;
  grid-template-columns: 24px 2fr 80px;
  align-items: center;
  grid-gap: 10px;
  overflow: hidden;
`

export const BlockCount = styled.span`
  font-size: 38px;
  line-height: 1;
  color: ${(p) => p.theme.color.text01};
  grid-column: 3;
  text-align: right;
`

export const BlockNote = styled.span`
  font-size: 14px;
  font-weight: 400;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
  grid-column: 2;
`

export const StatusBox = styled.section`
  padding: 22px 17px;
`

export const ControlBox = styled.div`
  display: grid;
  grid-template-columns: 24px 2fr 0.5fr;
  grid-gap: 10px;
  margin-bottom: 13px;
`

export const ShieldsIcon = styled.i<ShieldsIconProps>`
  --fill-color: ${(p) => p.theme.color.text02};
  width: 100%;
  height: auto;
  grid-column: 1;

  svg > path {
    fill: var(--fill-color);
  }

  ${p => p.isActive && css`
    --fill-color: ${(p) => p.theme.color.interactive02};
  `}
`

export const StatusText = styled.div`
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  grid-column: 2;
  word-break: break-word;
  display: -webkit-box;
  -webkit-line-clamp: 4;
  -webkit-box-orient: vertical;
  overflow: hidden;

  span {
    font-weight: 600;
  }
`

export const StatusToggle = styled.div`
  grid-column: 3;
  text-align: right;
`

export const StatusFootnoteBox = styled.div`
  display: grid;
  grid-template-columns: 24px 1fr;
  grid-gap: 10px;
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
`

export const Footnote = styled.div`
  color: ${(p) => p.theme.color.text01};
  text-align: left;
  grid-column: 2;

  span {
    color: ${(p) => p.theme.color.text03};
    display: block;
  }
`

export const ReportSiteBox = styled.div`
  grid-column: 2;

  p {
    max-width: 35ch;
    margin: 0 0 10px 0;
    color: ${(p) => p.theme.color.warningIcon};
  }
`

export const ReportSiteAction = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;

  span {
    flex: 1;
    color: ${(p) => p.theme.color.text03};
  }
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 0 0 25px 0;
  position: relative;
  z-index: 2;
`

export const AdvancedControlsButton = styled.button`
  --border: 3px solid transparent;
  --text-color: ${(p) => p.theme.color.text01};

  background-color: ${(p) => p.theme.color.background03};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  color: var(--text-color);
  width: 100%;
  padding: 10px 17px;
  border: var(--border);
  display: grid;
  grid-template-columns: 24px 1fr 18px;
  grid-gap: 10px;
  align-items: center;
  text-align: left;

  i { 
    grid-column: 1;
  }
  
  span { 
    grid-column: 2;
  }

  .icon-globe {
    path {
      fill: ${(p) => p.theme.color.interactive05};
    }
  }

  &:hover {
    --text-color: ${(p) => p.theme.color.interactive05};
  }

  &:focus-visible {
    --border: 3px solid ${(p) => p.theme.color.focusBorder};
  }
`

export const CaratIcon = styled(CaratStrongDownIcon)<CaratIconProps>`
  --rotate: rotate(0deg);

  width: 16px;
  height: 16px;
  transform: var(--rotate);

  ${p => p.isExpanded && css`
    --rotate: rotate(180deg);
  `}
`

export const GlobeIcon = styled.i`
  display: block;
  width: 17px;
  height: 17px;
  background-image: url(${globalIconUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  margin-right: 5px;
`
