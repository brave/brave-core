/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`

  & {
    --self-transition-duration: var(--search-transition-duration, 120ms);

    anchor-name: --search-box-anchor;
    color: ${color.text.primary};
    min-height: 48px;
  }

  .search-container {
    position: absolute;
    position-anchor: --search-box-anchor;
    inset: anchor(start) 0 auto;

    display: block;
    margin: 0 auto;
    overflow: visible;
    width: calc(100vw - 32px);
    max-width: 416px;

    transition-property: overlay, max-width, inset-block-start;
    transition-duration: var(--self-transition-duration);
    transition-timing-function: ease-out;
    transition-behavior: allow-discrete;

    &::backdrop {
      background: rgba(0, 0, 0, 0);
      transition: all var(--self-transition-duration) allow-discrete;
    }

    &:popover-open::backdrop {
      background: rgba(0, 0, 0, 0.2);

      @starting-style {
        background: rgba(0, 0, 0, 0);
      }
    }
  }

  &.expanded .search-container {
    inset-block-start: 27vh;
    max-width: 540px;
  }

  .input-container {
    anchor-name: --search-input-container;

    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px;
    border-radius: 12px;
    background: ${color.container.background};
    box-shadow: ${effect.elevation['03']};
    color: ${color.text.primary};

    &:hover, &:focus-within {
      box-shadow: ${effect.elevation['04']};
    }
  }

  input {
    flex-grow: 1;
    order: 2;
    border: none;
    padding: 0;
    font: inherit;
    outline: none;
    background: inherit;
  }

  .search-button {
    --leo-icon-size: 24px;

    order: 3;
    padding: 4px;
    border-radius: 4px;
    visibility: hidden;
    opacity: 0;
    color: ${color.icon.secondary};

    transition: opacity var(--self-transition-duration);

    &:hover {
      background-color: ${color.container.interactive};
    }
  }

  &.expanded .search-button {
    visibility: visible;
    opacity: 1;
  }

  .results-container {
    position: absolute;
    position-anchor: --search-input-container;
    position-area: bottom center;

    width: anchor-size(width);
    margin: 12px 0;
    display: flex;
    flex-direction: column;
    visibility: hidden;
    opacity: 0;

    border-radius: 16px;
    background: ${color.container.background};
    overflow: clip;
    box-shadow: ${effect.elevation['01']};

    transition: opacity var(--self-transition-duration);
  }

  &.expanded .results-container {
    visibility: visible;
    opacity: 1;
  }

`
