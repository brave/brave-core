/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  font-family: var(--brave-font-heading);
  font-size: 12px;
  line-height: 20px;
`

export const description = styled.div``

export const devices = styled.div`
  margin-top: 32px;

  table {
    width: 100%;
  }

  th, td {
    color: var(--brave-palette-neutral600);
  }

  th {
    padding: 3px 8px;
    text-align: left;
    font-weight: 500;
    border-bottom: 1px solid var(--brave-palette-neutral200);

    .icon {
      height: 12px;
      width: auto;
      vertical-align: middle;
      margin-left: 4px;
      margin-bottom: 3px;
    }
  }

  td {
    padding: 12px 8px;
  }

  tbody tr:first-child td {
    padding-top: 18px;
  }

  td.name {
    color: var(--brave-palette-neutral700);
    font-weight: 500;
    font-size: 14px;
    line-height: 20px;
  }

  td.edit {
    .icon {
      height: 12px;
      width: auto;
      position: relative;
      top: 1px;
      left: 0;
    }

    button {
      ${mixins.buttonReset}
      cursor: pointer;
      color: var(--brave-palette-neutral700);

      &:hover {
        color: var(--brave-palette-neutral900);
      }
    }

    input[type=text] {
      width: 112px;
      font-size: 14px;
      line-height: 20px;
      padding: 1px 10px;
      color: var(--brave-palette-neutral600);
      border: solid 1px var(--brave-palette-grey500);
      border-radius: 4px;
      outline: none;
    }
  }

  td.current {
    font-weight: 500;
  }
`

export const idTooltip = styled.div`
  display: none;
  position: absolute;
  top: 100%;
  left: 50%;
  transform: translate(-50%, 0);
  padding-top: 10px;

  &::before {
    content: ' ';
    display: block;
    position: absolute;
    top: 4px;
    left: calc(50% - 8px);
    transform: rotate(45deg);
    height: 20px;
    width: 20px;
    border-radius: 4px;
    z-index: -1;
    background: var(--brave-palette-black);
  }
`

export const idTooltipAnchor = styled.span`
  display: inline-block;
  position: relative;

  &:hover ${idTooltip} {
    display: block;
  }
`

export const idTooltipBody = styled.div`
  background: var(--brave-palette-black);
  color: var(--brave-palette-grey000);
  border-radius: 8px;
  width: 288px;
  padding: 18px;
  position: relative;
  left: 18px;
  box-shadow: 0 0 16px rgba(99, 105, 110, 0.18);
`

export const notLinked = styled.div`
  color: var(--brave-palette-neutral600);
  padding-bottom: 40px;
`

export const contactSupport = styled.span`
  a {
    font-weight: normal;
  }
`

export const actionBubbleAnchor = styled.span`
  position: relative;
`

export const actionMenu = styled.span`
  button {
    ${mixins.buttonReset}
    cursor: pointer;
    height: 18px;
    padding: 2px 4px;
    vertical-align: middle;
    margin-bottom: 1px;
    color: var(--brave-palette-neutral600);

    &:hover {
      color: var(--brave-palette-neutral700);
    }

    .icon {
      height: 100%;
      width: auto;
    }
  }
`

export const actionBubble = styled.div`
  position: absolute;
  top: 0;
  right: calc(100% - 2px);
  padding: 9px 20px;
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-neutral700);
  box-shadow: 0 0 16px rgba(99, 105, 110, 0.18);
  background: var(--brave-palette-white);
  z-index: 11;
  white-space: nowrap;
  border-radius: 6px;

  button {
    ${mixins.buttonReset}
    font-weight: 600;
    color: var(--brave-palette-blurple500);
    cursor: pointer;

    &[disabled] {
      color: var(--brave-palette-neutral600);
      font-weight: normal;
      cursor: default;
    }
  }

  .icon {
    height: 14px;
    width: auto;
    vertical-align: middle;
    margin-right: 9px;
    margin-bottom: 2px;
  }
`

export const bubbleBackdrop = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  z-index: 10;
`

export const nextUnlinking = styled.div`
  margin-top: 19px;
  color: var(--brave-palette-red500);
  text-align: center;
  font-weight: 500;
`
