/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const buttonReset = `
  margin: 0;
  padding: 0;
  background: 0;
  border: none;
  font-size: inherit;
  line-height: inherit;
`

export const tooltipAnchor = `
  position: relative;

  .tooltip {
    position: absolute;
    z-index: 1;
    visibility: hidden;
    transition: visibility 0s linear 300ms;
  }

  &:hover .tooltip {
    visibility: initial;
  }
`

export const tooltipContainer = `
  position: relative;

  &:before {
    content: ' ';
    position: absolute;
    background: inherit;
    height: 15px;
    width: 15px;
    transform: rotate(45deg);
  }
`
