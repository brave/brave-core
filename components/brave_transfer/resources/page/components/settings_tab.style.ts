/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
  }

  section {
    display: flex;
    flex-direction: column;
    border-radius: ${radius.l};
    border: 1px solid ${color.divider.subtle};
    background: ${color.container.background};
  }

  h2 {
    padding: ${spacing.l} ${spacing.xl} 0;
    font: ${font.heading.h4};
  }

  section > .setting-description {
    padding: ${spacing.l} ${spacing.xl};
  }

  .setting {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.xl};
    padding: ${spacing.l} ${spacing.xl};
  }

  .setting + .setting {
    border-top: 1px solid ${color.divider.subtle};
  }

  .setting-text,
  .trusted-device-info {
    display: flex;
    flex-direction: column;
    gap: ${spacing.xs};
    min-width: 0;
  }

  .setting-label {
    font: ${font.default.semibold};
  }

  .setting-description {
    font: ${font.small.regular};
    color: ${color.text.secondary};
  }

  .path {
    display: flex;
    align-items: center;
    gap: ${spacing.s};
    overflow-wrap: anywhere;
    --leo-icon-size: 16px;
  }

  .setting-actions,
  .trusted-device-actions {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    gap: ${spacing.s};
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .trusted-device {
    display: flex;
    align-items: center;
    gap: ${spacing.l};
    padding: ${spacing.l} ${spacing.xl};
  }

  .trusted-device + .trusted-device {
    border-top: 1px solid ${color.divider.subtle};
  }

  .trusted-device-icon {
    flex: 0 0 auto;
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.icon.secondary};
  }

  .trusted-device-info {
    flex: 1 1 auto;
  }

  .fingerprint {
    font: ${font.monospace.small};
    color: ${color.text.tertiary};
  }

  .trusted-device-actions leo-dropdown {
    min-width: 150px;
  }

  @media (max-width: 640px) {
    .save-location,
    .trusted-device {
      flex-wrap: wrap;
    }

    .trusted-device-actions {
      flex: 1 1 100%;
      justify-content: space-between;
    }
  }
`
