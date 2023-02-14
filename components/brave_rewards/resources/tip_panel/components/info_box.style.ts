/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  align-items: flex-start;
  gap: 18px;
  background: var(--leo-color-systemfeedback-info-background);
  border-radius: 8px;
  padding: 24px;

  .icon {
    color: var(--leo-color-systemfeedback-info-icon);
  }

  &.warn {
    background: var(--leo-color-systemfeedback-warning-background);

    .icon {
      color: var(--leo-color-systemfeedback-warning-icon);
    }
  }

  &.error {
    background: var(--leo-color-systemfeedback-error-background);

    .icon {
      color: var(--leo-color-systemfeedback-error-icon);
    }
  }
`

export const icon = styled.div`
  flex: 0 0 26px;
  opacity: 0.25;
`

export const content = styled.div`
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  gap: 4px;
`

export const title = styled.div`
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: var(--leo-color-text-primary);
`

export const text = styled.div`
  font-size: 12px;
  line-height: 18px;
  color: var(--leo-color-text-primary);

  > div:not(:first-child) {
    margin-top: 1em;
  }
`
