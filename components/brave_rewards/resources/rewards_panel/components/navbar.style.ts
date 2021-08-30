/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  background: var(--brave-palette-white);
  box-shadow: 0px 4px 16px rgba(27, 29, 47, 0.08);
  border-radius: 16px;
  color: var(--brave-palette-neutral600);
  font-size: 14px;
  line-height: 18px;
  margin-top: 13px;
  padding: 24px 30px;
  display: flex;

  button {
    font-weight: inherit;
    border: none;
    background: none;
    margin: 0;
    padding: 0;
    cursor: pointer;

    &:hover {
      color: var(--brave-palette-blurple400);
    }

    &:active {
      color: var(--brave-color-brandBatInteracting);
    }

    &[disabled] {
      cursor: default;
      color: inherit;
    }
  }

  .icon {
    width: 20px;
    height: auto;
    vertical-align: middle;
    margin-bottom: 2px;
    margin-right: 2px;
  }

  .selected button {
    font-weight: 600;
    color: var(--brave-color-brandBatInteracting);
  }

  .brave-theme-dark & {
    background: #1E2029;
    color: #999EA2;

    button:hover {
      color: var(--brave-palette-grey100);
    }

    .selected button {
      color: var(--brave-palette-blurple400);
    }
  }
`

export const tip = styled.div`
  flex: 1 1 auto;
  margin-left: 38px;

  .icon {
    width: 24px;
  }
`

export const summary = styled.div`
  flex: 1 1 auto;
  text-align: right;
  margin-right: 17px;

  .icon {
    margin-bottom: 4px;
  }
`

export const settings = styled.div`
  flex: 0 0 auto;
  border-left: solid 1px var(--brave-palette-neutral200);
  padding-left: 16px;
  height: 24px;

  .icon {
    margin: 1px 0 0;
  }

  .brave-theme-dark & {
    border-color: var(--brave-palette-grey800);
    color: #999EA2;
  }
`
