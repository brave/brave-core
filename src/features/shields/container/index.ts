/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ComponentType } from 'react'
import styled from '../../../theme'
import palette from '../../../theme/palette'

// Extendable components
// ..............................
const Grid = styled<any, any>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  font-family: ${p => p.theme.fontFamily.heading};
  display: grid;
  grid-template-columns: auto auto;
  grid-template-rows: 1fr;
  grid-gap: 0;
  justify-content: center;
  align-items: center;
`

const Flex = styled<any, any>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  font-family: ${p => p.theme.fontFamily.heading};
  display: flex;
  align-items: center;
`

// Main wrapper
// ..............................
export const ResourcesGrid = styled(Grid)`
  box-sizing: border-box;
  position: absolute;
  z-index: 1;
  top: 0;
  left: 0;
  background: linear-gradient(to bottom, #131526, #343546);
  width: 100%;
  height: 100%;
  grid-template-columns: 1fr;
  grid-template-rows: auto auto 1fr;
`

// Header
// ..............................
interface MainToggleFlexProps {
  enabled: boolean
}

export const MainToggleFlex = styled(Flex as ComponentType<MainToggleFlexProps>)`
  justify-content: space-between;
  padding: ${p => p.enabled ? '0' : '0 0 22px'};
`

export const MainSiteInfoGrid = styled(Grid)`
  grid-gap: 5px;
  margin: 0 0 5px;
`

export const ResourcesSiteInfoFlex = styled(Flex)`
  justify-content: space-between;
  padding: 25px 25px 15px;
`

export const ResourcesSiteInfoGrid = styled(Grid)`
  grid-template-columns: auto 1fr;
  grid-gap: 5px;
`

export const EnabledTextGrid = styled(Grid)`
  grid-gap: 5px;
  justify-content: center;

  &:first-child {
    margin: 0 0 5px;
  }
`

export const DisabledTextGrid = styled(EnabledTextGrid)`
  grid-template-columns: 2fr 4fr;
  max-width: 90%;
  margin: 10px auto 10px;
`

interface ResourcesStatusGridProps {
  withStats: boolean
  onClick: (event: React.MouseEvent<HTMLButtonElement>) => void
}

export const ResourcesStatusGrid = styled(Grid as ComponentType<ResourcesStatusGridProps>)`
  grid-template-columns: ${p => p.withStats ? '28px 28px 1fr' : '28px 1fr'};
  padding: 5px 25px 5px 20px;
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: #E9E9F4;
  background-color: rgba(255, 255, 255, 0.15);
  user-select: none;
  cursor: pointer;

  > *:nth-child(2) {
    padding: 0;
  }
`

export const ResourcesSubTitleGrid = styled(Grid)`
  position: sticky;
  top: 0;
  grid-template-columns: auto 1fr auto;
  grid-template-rows: auto;
  padding: 5px 25px 5px 20px;
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: #E9E9F4;
  height: auto;
  background: ${palette.grey800};
`

// Footer
// ..............................
export const MainFooterLinkFlex = styled(Flex.withComponent('a'))`
  justify-content: space-between;
  padding: 20px 25px;
  color: ${palette.blue200};
  font-size: 13px;
  font-weight: 600;
  text-decoration: none;

  &:hover {
    color: ${palette.white};
  }
`

export const ResourcesFooterGrid = styled(Grid.withComponent('footer'))`
  grid-template-columns: 1fr 1fr;
  border-top: 1px solid rgba(255,255,255,0.15);
`

export const ResourcesFooterGridColumnLeft = styled(Flex)`
  justify-content: flex-start;
  height: 65px;
  padding-left: 20px;
`

export const ResourcesFooterGridColumnRight = styled(Flex)`
  justify-content: flex-end;
  height: 65px;
  padding-right: 20px;
  position: relative;
`

export const ResourcesFooterFlex = styled(Flex)`
  justify-content: center;
  margin: auto;
  padding: 15px 0;
  border-top: 1px solid rgba(255,255,255,0.15);
  width: 100%;
`

// Content
// ..............................

export const StatFlex = styled(Flex)`
  box-sizing: border-box;
  width: 100%;
  height: 100%;
  font-family: ${p => p.theme.fontFamily.heading};
  color: ${palette.grey200};
  font-size: 14px;
  font-weight: 600;
  line-height: 1;
  user-select: none;
  padding: 13px 0;
`

export const ToggleGrid = styled(Grid)`
  grid-template-columns: 48px 28px 1fr auto;
  padding: 0;
  border-bottom: 1px solid rgba(255,255,255,0.15);
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: #E9E9F4;

  &:hover {
    background-color: rgba(255, 255, 255, 0.15);
  }

  > *:first-child {
    cursor: pointer;
    padding-left: 20px;
  }

  > *:nth-child(2) {
    cursor: pointer;
    height: 40px;
  }

  > *:nth-child(3) {
    cursor: pointer;
  }
`

interface SelectGridProps {
  hasUserInteraction: boolean
}

export const SelectGrid = styled(ToggleGrid as ComponentType<SelectGridProps>)`
  padding: 0;
  cursor: pointer;

  > *:nth-child(2) {
    height: 46px;
  }

  &:hover {
    cursor: ${p => p.hasUserInteraction ? 'pointer' : 'unset'};
    background-color: ${p => p.hasUserInteraction ? 'rgba(255, 255, 255, 0.15)' : 'transparent'};
  }
`

interface ResourcesListGridProps {
  hightlighted: boolean
}

export const ResourcesListGrid = styled(Grid as ComponentType<ResourcesListGridProps>)`
  grid-template-columns: auto 1fr auto;
  grid-template-rows: auto;
  padding: 9px 25px 9px 20px;
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: #E9E9F4;
  background-color: ${p => p.hightlighted ? 'rgba(255, 255, 255, 0.15)' : null};
  &:hover {
    background-color: rgba(255, 255, 255, 0.15);
  }
`

export const ToggleFlex = styled(Flex.withComponent('label'))`
  width: 100%;
  height: 100%;
  padding-right: 25px;
`

export const ShieldIconFlex = styled(Flex)`
  display: flex;
  justify-content: center;
`
