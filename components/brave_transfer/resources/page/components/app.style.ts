/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    color-scheme: light dark;
    min-height: 100vh;
    background: ${color.page.background};
    padding: ${spacing['3Xl']} ${spacing.xl};
  }

  &[data-mobile=true] {
    padding: ${spacing.l};
  }

  .content {
    display: flex;
    flex-direction: column;
    gap: ${spacing.xl};
    max-width: 880px;
    margin: 0 auto;
  }

  header {
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
  }

  .title {
    display: flex;
    align-items: center;
    gap: ${spacing.m};
    --leo-icon-size: 28px;
    --leo-icon-color: ${color.icon.interactive};
  }

  h1 {
    font: ${font.heading.h2};
  }

  .description {
    color: ${color.text.secondary};
  }

  .note {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
  }

  .tabs {
    align-self: flex-start;
  }

  @media (max-width: 640px) {
    & {
      padding: ${spacing.l};
    }

    h1 {
      font: ${font.heading.h3};
    }

    .tabs {
      align-self: stretch;
      --leo-segmented-control-width: 100%;
    }
  }
`

style.passthrough.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
  }

  h1, h2, h3, h4, p {
    margin: 0;
    text-wrap: pretty;
  }

  /* Leo buttons grow to fill flex rows unless told otherwise. */
  leo-button {
    flex-grow: 0;
  }
`
