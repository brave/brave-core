/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div.attrs({
  'data-theme': 'light'
})`
  position: fixed;
  z-index: 100;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  font-family: Poppins;
  background: ${leo.color.container.background};
  overflow: auto;
  padding: 32px 30px;
  display: flex;
  flex-direction: column;
  align-items: center;

  --self-content-max-width: 607px;
  --self-text-max-width: 479px;

  a {
    color: ${leo.color.text.interactive};
    text-decoration: underline;
  }
`

export const branding = styled.div`
  width: 100%;
  display: flex;
  gap: 2px;
  align-items: center;

  .logo {
    --leo-icon-size: 36px;
  }

  .logo-text .icon {
    display: block;
    height: 18px;
    width: auto;
  }
`

export const content = styled.div`
  max-width: var(--self-content-max-width);
  display: flex;
  flex-direction: column;
  align-items: center;
`

export const nav = styled.div`
  width: 100%;
  margin-top: 8px;
`

export const title = styled.div.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.text.primary};
  font-size: 28px;
  font-weight: 500;
  line-height: 40px;
  margin-top: 32px;
  padding: 8px 0;
  text-align: center;
`

export const text = styled.div.attrs({
  'data-theme': 'light'
})`
  width: 100%;
  max-width: var(--self-text-max-width);
  color: ${leo.color.text.secondary};
  font-size: 14px;
  line-height: 24px;
  padding: 8px 0;
`

export const providerSelection = styled.div`
  width: 100%;
  max-width: var(--self-text-max-width);
  padding: 32px 0;

  .layout-narrow & {
    padding: 8px 0;
  }
`

export const providerGroupHeader = styled.div.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.text.secondary};
  font-size: 14px;
  line-height: 24px;

  ${mixins.tooltipAnchor}

  .tooltip {
    top: 100%;
    left: 50%;
    transform: translateX(-50%);
    padding-top: 6px;
  }

  /* Tooltips are anchored to this element, but triggered by a child element */
  &:hover .tooltip {
    visibility: hidden;
  }
`

export const providerGroupHeaderIcon = styled.span.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.icon.default};

  --leo-icon-size: 20px;

  leo-icon {
    display: inline-block;
    vertical-align: text-bottom;
    margin-left: 8px;
  }

  &:hover .tooltip {
    visibility: initial;
  }
`

export const providerGroupTooltip = styled.div.attrs({
  'data-theme': 'light'
})`
  ${mixins.tooltipContainer}

  width: 270px;
  padding: 16px;
  font-size: 14px;
  line-height: 22px;
  color: ${leo.color.text.primary};
  border-radius: 8px;
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.08);
  background: ${leo.color.white};

  &:before {
    height: 9px;
    width: 9px;
    top: -3px;
    left: calc(50% - 1px);
  }
`

export const providerGroup = styled.div.attrs({
  'data-theme': 'light'
})`
  margin-top: 8px;
  border-radius: 8px;
  border: 1px solid ${leo.color.divider.subtle};
  display: flex;
  flex-direction: column;
  padding: 0 24px;

  button {
    ${mixins.buttonReset};
    padding: 16px 0;
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: 11px;
    text-align: left;
    border-top: solid 1px ${leo.color.divider.subtle};

    --self-icon-color: ${leo.color.icon.default};

    &:first-child {
      border-top: none;
    }

    &:disabled {
      cursor: default;
    }

    &:hover {
      --self-icon-color: ${leo.color.icon.interactive};
    }
  }
`

export const providerButtonIcon = styled.div`
  padding: 10px;

  .icon {
    height: 32px;
  }
`

export const providerButtonName = styled.div.attrs({
  'data-theme': 'light'
})`
  flex: 1 1 auto;
  color: ${leo.color.text.primary};
  font-size: 16px;
  font-weight: 600;
  line-height: 24px;
`

export const providerButtonMessage = styled.div.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.text.secondary};
  font-size: 12px;
  line-height: 18px;
`

export const providerButtonCaret = styled.div`
  color: var(--self-icon-color);
  --leo-icon-size: 24px;
`

export const regionsLearnMore = styled.div`
  margin-top: 8px;
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
  letter-spacing: 0.12px;
`
