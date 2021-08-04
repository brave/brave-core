/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  position: absolute;
  top: 32px;
  left: 1px;
  overflow: hidden;
  z-index: 1;
  width: 329px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.28);
  border: solid 1px rgba(0, 0, 0, .15);
  border-radius: 6px;
`

export const content = styled.div`
  background: var(--brave-palette-white);
  color: var(--brave-palette-neutral900);
  padding: 14px 21px 16px;

  .brave-theme-dark & {
    background: var(--brave-palette-grey800);
    color: var(--brave-palette-grey000);
  }
`

export const header = styled.div`
  border-bottom: solid 1px var(--brave-palette-neutral200);
  padding-bottom: 7px;
  display: flex;

  .brave-theme-dark & {
    border-color: var(--brave-palette-grey700);
  }
`

export const providerIcon = styled.div`
  .icon {
    height: 24px;
    width: auto;
    vertical-align: middle;
    margin-right: 8px;
    margin-bottom: 2px;
  }
`

export const username = styled.div`
  flex: 1 1 auto;
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;

  .brave-theme-dark & {
    color: var(--brave-palette-white);
  }
`

export const status = styled.div`
  font-size: 14px;
  line-height: 20px;

  &.verified {
    color: var(--brave-palette-teal600);
  }

  .icon {
    height: 16px;
    width: auto;
    vertical-align: middle;
    margin-right: 6px;
    margin-bottom: 2px;
  }
`

export const pendingNotice = styled.div`
  font-size: 12px;
  line-height: 18px;
  padding: 14px 0 9px;
  border-bottom: solid 1px var(--brave-palette-neutral200);
  display: flex;

  .icon {
    flex: 0 0 auto;
    height: 16px;
    width: auto;
    vertical-align: middle;
    margin-right: 6px;
    margin-bottom: 2px;
  }

  .text {
    flex: 1 1 auto;
    margin-left: 9px;
  }

  .brave-theme-dark & {
    border-color: var(--brave-palette-grey700);
  }
`

export const links = styled.div`
  margin: 11px 4px 0;
`

export const link = styled.div`
  a {
    font-size: 12px;
    line-height: 24px;
    color: var(--brave-color-brandBat);
    text-decoration: none;

    &:hover {
      text-decoration: underline;
    }

    .brave-theme-dark & {
      color: var(--brave-palette-blurple300);
    }
  }
`

export const linkMarker = styled.div`
  border-radius: 50%;
  display: inline-block;
  height: 5px;
  width: 5px;
  background-color: var(--brave-palette-grey500);
  margin-bottom: 2px;
  margin-right: 8px;

  .brave-theme-dark & {
    background-color: var(--brave-palette-grey600);
  }
`

export const backdrop = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  z-index: -1;
`
