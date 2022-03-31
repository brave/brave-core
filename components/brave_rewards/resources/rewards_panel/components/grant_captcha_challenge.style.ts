/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  text-align: center;
`

export const loading = styled.div`
  height: 462px;
  display: flex;
  justify-content: center;
  align-items: center;
  flex-direction: column;

  .icon {
    height: 32px;
    width: auto;
    margin-bottom: 150px;
    color: var(--brave-color-brandBat);

    animation-name: fade-in;
    animation-delay: 200ms;
    animation-duration: 500ms;
    animation-fill-mode: both;

    @keyframes fade-in {
      from { opacity: 0; }
      to { opacity: .8; }
    }
  }
`

export const hint = styled.div`
  padding: 8px;
  font-size: 14px;
  line-height: 18px;
  border-radius: 4px;
  background: var(--brave-palette-neutral000);
  color: var(--brave-palette-neutral700);
`

export const dragObject = styled.div`
  margin-top: 10px;

  img {
    width: 60px;
    height: 52px;
  }
`

export const dragObjectCircle = styled.div`
  display: inline-block;
  border: 2px solid var(--brave-palette-neutral200);
  border-radius: 50%;
  height: 84px;
  width: 84px;
  padding-top: 8px;
`

export const arrow = styled.div`
  margin-top: 10px;

  .icon {
    height: 20px;
    width: auto;
    color: var(--brave-palette-grey200);
  }
`

export const dragTarget = styled.div`
  text-align: center;

  img {
    width: 333px;
    height: auto;
  }
`
