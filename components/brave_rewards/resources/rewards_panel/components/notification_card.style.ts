/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  border-radius: 16px;
  padding: 9px 18px 20px;
  font-family: var(--brave-font-heading);

  strong {
    font-weight: 600;
  }

  .brave-theme-dark & {
    background: var(--brave-palette-grey800);
  }
`

export const header = styled.div`
  display: flex;
  justify-content: space-between;
`

export const date = styled.div`
  border-radius: 48px;
  border: solid 1px var(--brave-palette-neutral200);
  padding: 3px 14px;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-neutral700);
  text-transform: uppercase;

  .icon {
    height: 14px;
    width: auto;
    vertical-align: middle;
    margin-bottom: 2px;
    margin-right: 8px;
    color: var(--brave-palette-neutral600);
  }

  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
    border-color: var(--brave-palette-grey700);

    .icon {
      color: var(--brave-palette-grey500);
    }
  }
`

export const close = styled.div`
  text-align: right;
  padding: 5px;

  button {
    ${mixins.buttonReset}
    color: var(--brave-palette-grey500);
    cursor: pointer;

    .brave-theme-dark & {
      color: var(--brave-palette-grey700);
    }

    &:active {
      color: var(--brave-palette-grey600);
    }
  }

  .icon {
    height: 12px;
    width: auto;
  }
`

const actionButtonMixin = `
  ${mixins.buttonReset}
  width: 100%;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  border-radius: 48px;
  padding: 10px;
  cursor: pointer;
`

export const content = styled.div`
  text-align: center;
  font-size: 14px;
  line-height: 22px;
`

export const title = styled.div`
  margin: 20px 0 9px;
  font-weight: 500;
  font-size: 18px;
  line-height: 20px;
  color: var(--brave-palette-neutral900);
  text-align: center;

  .brave-theme-dark & {
    color: var(--brave-palette-grey000);
  }

  .icon {
    height: 16px;
    width: auto;
    vertical-align: middle;
    margin-right: 5px;
    margin-bottom: 3px;
    color: var(--brave-palette-green500);
  }

  &.funding .icon {
    color: var(--brave-palette-green500);
  }

  &.information .icon {
    color: var(--brave-palette-blue500);
  }

  &.error .icon {
    color: var(--brave-palette-red500);
  }
`

export const body = styled.div`
  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
  }
`

export const action = styled.div`
  margin-top: 16px;
  text-align: center;

  button {
    ${actionButtonMixin}
    color: var(--brave-palette-white);
    background: var(--brave-color-brandBat);

    &:active {
      background: var(--brave-palette-blurple400);
    }

    .brave-theme-dark & {
      color: var(--brave-palette-grey000);
    }
  }
`
