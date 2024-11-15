/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`

  & {
    --self-transition-duration: 200ms;

    anchor-name: --search-box-anchor;
    color: ${color.text.primary};
  }

  .search-container {
    position: absolute;
    position-anchor: --search-box-anchor;
    inset: anchor(start) 0 auto;

    display: block;
    margin: 0 auto;
    overflow: visible;
    width: calc(100vw - 32px);
    max-width: 393px;

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

  /* Transitioning inset-block-start is causing a render crash when using anchor
     positioning and transitioning from display: none to display: block. */
  &.hidden .search-container {
    visibility: hidden;
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
    color: ${color.text.primary};

    &:hover, &:focus-within {
      box-shadow: ${effect.elevation['01']};
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

  .engine-picker-button {
    --leo-icon-size: 16px;

    anchor-name: --engine-picker-button;

    order: 1;
    padding: 7px;
    border-radius: 4px;
    border: solid 1px transparent;

    &:hover {
      background-color: ${color.container.interactive};
    }

    &.open {
      background-color: ${color.container.interactive};
      border-color: ${color.divider.interactive};
    }
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

  .engine-options {
    --leo-icon-size: 20px;

    position: absolute;
    position-anchor: --engine-picker-button;
    position-area: block-end span-inline-end;
    margin-top: 2px;
    min-width: 232px;
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

    transition: opacity var(--self-transition-duration);
  }

  &.expanded .results-container {
    visibility: visible;
    opacity: 1;
  }

`
