/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

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
  overscroll-behavior: contain;
  font: ${leo.font.default.regular};
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
  font: ${leo.font.heading.h2};
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
  padding: 8px 0;
`

export const providerGroups = styled.div`
  width: 100%;
  max-width: var(--self-text-max-width);
  padding: 32px 0;
  display: flex;
  flex-direction: column;
  gap: 24px;

  .layout-narrow & {
    padding: 8px 0;
  }
`

export const providerGroupHeader = styled.div.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.text.secondary};
  font: ${leo.font.default.regular};
  display: flex;
  gap: 8px;
  align-items: center;
`

export const providerGroupHeaderIcon = styled.span.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.icon.default};

  ${mixins.tooltipAnchor}

  .tooltip {
    top: 100%;
    left: 0;
    padding-top: 6px;
  }

  .custodial.tooltip {
    left: -107px;
    transform: translateX(-50%);
  }

  .self-custody.tooltip {
    left: 55px;
    transform: translateX(-50%);
  }

  --leo-icon-size: 18px;

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
  color: ${leo.color.text.primary};
  border-radius: 8px;
  border: solid 1px ${leo.color.divider.subtle};
  box-shadow: 0px 3px 10px -1px rgba(0, 0, 0, 0.05);
  background: ${leo.color.white};
  display: flex;
  flex-direction: column;
  gap: 5px;

  a {
    font: ${leo.font.components.buttonDefault};
    text-decoration: none;
  }

  &:before {
    height: 9px;
    width: 9px;
    top: -5px;
    border: solid 1px ${leo.color.divider.subtle};
    border-bottom-color: transparent;
    border-right-color: transparent;
  }

  .custodial &:before {
    right: 13.5px;
  }

  .self-custody & {
    width: 310px;
  }

  .self-custody &:before {
    left: 103.5px;
  }
`

export const providerGroupItems = styled.div.attrs({
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
    position: relative;
    padding: 16px 0;
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: 11px;
    text-align: left;
    border-top: solid 1px ${leo.color.divider.subtle};

    &:first-child {
      border-top: none;
    }

    &:disabled {
      cursor: default;
    }
  }
`

export const providerButtonIcon = styled.div`
  padding: 10px;

  --leo-icon-size: 32px;

  .icon {
    height: 32px;
    width: auto;
    vertical-align: middle;
  }

  button:disabled & {
    --provider-icon-color: ${leo.color.icon.disabled};
  }
`

export const providerButtonName = styled.div.attrs({
  'data-theme': 'light'
})`
  flex: 1 1 auto;
  color: ${leo.color.text.primary};
  font: ${leo.font.large.semibold};

  button:disabled & {
    color: ${leo.color.text.tertiary};
  }
`

export const providerButtonMessage = styled.div.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.text.tertiary};
  font: ${leo.font.small.regular};
`

export const providerButtonCaret = styled.div.attrs({
  'data-theme': 'light'
})`
  display: flex;
  align-items: center;
  font: ${leo.font.components.buttonSmall};
  color: ${leo.color.icon.interactive};
  --leo-icon-size: 24px;
  --leo-progressring-size: 24px;
`

export const providerButtonCaretText = styled.div`
  .layout-narrow & {
    display: none;
  }
`

export const regionsLearnMore = styled.div`
  margin-top: 8px;
  color: ${leo.color.text.tertiary};
  font: ${leo.font.small.link};

  a {
    color: inherit;
  }
`

export const connectError = styled.div.attrs({
  'data-theme': 'light'
})`
  margin-top: 24px;
  background: ${leo.color.systemfeedback.warningBackground};
  color: ${leo.color.systemfeedback.warningText};
  border-radius: 8px;
  padding: 16px;
  display: flex;
  gap: 16px;
  align-items: center;

  --leo-icon-size: 20px;
  --leo-icon-color: ${leo.color.systemfeedback.warningIcon};
`

export const selfCustodyNote = styled.div`
  margin-top: 24px;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.secondary};
`

export const newBadge = styled.div.attrs({
  'data-theme': 'light'
})`
  position: absolute;
  top: 10px;
  left: -14px;
  padding: 4px;
  border-radius: 4px;
  background: ${leo.color.primary['20']};
  color: ${leo.color.primary['50']};
  font: ${leo.font.components.label};
`
