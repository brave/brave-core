/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledListContent = styled('div')<{}>`
  padding: 0 25px;
`

export const StyledSitesNum = styled('div')<{}>`
  height: 50px;
  padding: 20px 25px;
  margin-top: -21px;
`

export const StyledDisabledContent = styled('div')<{}>`
  padding: 0px 5px;
`

export const StyledHeading = styled('span')<{}>`
  font-size: 22px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 28px;
`

export const StyledSitesLink = styled('a')<{}>`
  float: right;
  color: #4C54D2;
  font-size: 13px;
  letter-spacing: 0;
`

export const StyledText = styled('p')<{}>`
  color: #838391;
  font-size: 14px;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: 300;
  letter-spacing: 0;
  line-height: 28px;
`

export const StyledTotalContent = styled('div')<{}>`
  position: relative;
  padding-right: 25px;
  text-align: right;

  @media (max-width: 366px) {
    top: 11px;
  }
`

export const StyledWalletOverlay = styled('div')<{}>`
  display: flex;
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: ${p => p.theme.color.modalOverlayBackground};
  align-items: center;
  z-index: 999;
  justify-content: center;
`

export const StyledWalletWrapper = styled('div')<{}>`
  height: 90vh;
  overflow-y: scroll;
  width: 90%;
  position: absolute;
  top: 50px;

  @media (max-height: 515px) {
    max-height: 415px;
    overflow-y: scroll;
  }
`

export const StyledWalletClose = styled('div')<{}>`
  top: 15px;
  right: 15px;
  position: fixed;
  color: ${p => p.theme.color.subtleExclude};
  width: 25px;
`

export const StyledArrivingSoon = styled.div`
  background: rgba(93, 181, 252, 0.2);
  margin-bottom: 8px;
  padding: 4px;
  color: var(--brave-palette-neutral700);
  text-align: center;
  font-size: 12px;
  line-height: 22px;

  span.amount {
    color: var(--brave-palette-neutral900);
    font-weight: 600;
    font-size: 14px;
    line-height: 24px;
  }

  .icon {
    height: 16px;
    width: auto;
    fill: var(--brave-palette-blue500);
    vertical-align: middle;
    margin-bottom: 3px;
    margin-right: 8px;
  }
`
