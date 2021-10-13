/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const tooltipMixin = `
  position: relative;
  background: var(--brave-palette-blurple500);
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 6px;
  color: var(--brave-palette-white);
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;

  &:before {
    content: '';
    position: absolute;
    bottom: -7px;
    left: 23px;
    background: inherit;
    height: 15px;
    width: 15px;
    transform: rotate(45deg);
  }
`
