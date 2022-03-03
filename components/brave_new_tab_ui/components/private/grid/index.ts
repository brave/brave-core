/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'

interface Props {
  isStandalonePrivatePage?: boolean
  isTorCircuitInitializing?: boolean
  isTorCircuitEstablishedOrInitializing?: boolean
}

export const Grid = styled('section')<{}>`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  grid-template-columns: 50% 50%;
  grid-template-areas:
    "header header"
    "content1 content2";

  grid-auto-rows: auto;
  grid-gap: 40px 30px;
  padding: 0;
  align-items: center;

  > *:first-child {
    grid-area: header;
  }

  > *:nth-child(2) {
    grid-area: content1;
  }

  > *:nth-child(3) {
    grid-area: content2;
  }

  @media screen and (max-width: 1170px) {
    grid-template-columns: 460px;
    justify-content: center;
    grid-template-areas:
    "header"
    "content1"
    "content2";
  }

  @media screen and (max-width: 490px) {
    /* TODO: @cezaraugusto this needs more love */
    zoom: 0.8;
  }
`

export const Grid2Columns = styled('section')<{}>`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  padding: 0;
  align-items: center;
  grid-template-columns: 50% 50%;
  grid-gap: 40px 30px;
  grid-template-areas: "content1 content2";

  > *:first-child {
    grid-area: content1;
  }

  > *:nth-child(2) {
    grid-area: content2;
  }

  @media screen and (max-width: 1170px) {
    padding: 25px;
    grid-template-columns: 1fr;
    grid-template-areas:
    "image"
    "content";
  }

  @media screen and (max-width: 1170px) {
    grid-template-columns: 460px;
    justify-content: center;
    grid-template-areas:
    "header"
    "content1"
    "content2";
  }
`

export const Grid3Columns = styled('section')<{}>`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  grid-template-columns: 75px 297px 297px;
  grid-template-areas:
    "header header header"
    "image content1 content2";

  grid-auto-rows: auto;
  grid-gap: 20px 20px;
  padding: 0;
  justify-content: center;

  > *:first-child {
    grid-area: header;
  }

  > *:nth-child(2) {
    grid-area: image;
  }

  > *:nth-child(3) {
    grid-area: content1;
  }

  > *:nth-child(4) {
    grid-area: content2;
  }

  @media screen and (max-width: 1170px) {
    grid-template-columns: 460px;
    grid-template-areas:
    "header"
    "content1"
    "content2";

    > *:nth-child(2) {
      display: none;
    }
  }

  @media screen and (max-width: 490px) {
    /* TODO: @cezaraugusto this needs more love */
    zoom: 0.8;
  }
`

export const HeaderGrid = styled('section')<Props>`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  grid-template-columns: ${p => p.isStandalonePrivatePage ? '1fr' : 'auto 1fr'};
  ${p => !p.isStandalonePrivatePage && 'grid-template-areas: "image content"'};
  ${p => p.isStandalonePrivatePage && css`
    grid-template-areas:
      "image"
      "content";
  `}

  grid-auto-rows: auto;
  grid-gap: ${p => p.isStandalonePrivatePage ? '10px' : '50px'};
  padding: 0 50px;
  align-items: center;

  > *:first-child {
    grid-area: image;
  }

  > *:nth-child(2) {
    grid-area: content;
  }

  @media screen and (max-width: 1170px) {
    padding: 25px;
    grid-template-columns: 1fr;
    grid-template-areas:
    "image"
    "content";
  }
`

export const ButtonGroup = styled('footer')<{}>`
  display: flex;
  flex: 1;
  justify-content: flex-start;
`

export const ToggleGroup = styled('footer')<{}>`
  display: flex;
  flex: 1;
  justify-content: space-between;
  align-items: center;
`

export const IconText = styled('div')<{}>`
  display: flex;
  justify: space-between;
  align-items: center;
`

export const TorStatusGrid = styled('div')<{}>`
  display: grid;
  justify-content: center;
`

export const TorStatusContainer = styled('div')<Props>`
  width: ${p => p.isTorCircuitEstablishedOrInitializing ? '246px' : '158px'};
  height: 32px;
  line-height: 32px;
  background: rgba(227, 36, 68, 0.2);
  border-radius: 39px;
  margin-top: 70px;
`

export const TorStatusIndicator = styled('img')<Props>`
  width: 10px;
  height: 10px;
  margin-left: ${p => p.isTorCircuitInitializing ? '30px' : '15px'};
  margin-right: 11px;
`

export const TorStatusText = styled('div')<{}>`
  display: inline-block;
  text-align: center;
  font-family: Poppins;
  font-size: 14px;
  font-weight: 500;
  letter-spacing: 0.01em;
  line-height: 20px;
  color: #FFFFFF;
`

export const TorHelpText = styled('div')<{}>`
  position: absolute;
  left: 39.46%;
  right: 39.52%;
  text-align: center;
  font-family: Poppins;
  font-size: 12px;
  font-weight: 500;
  letter-spacing: 0.01em;
  line-height: 18px;
  margin-top: 17px;
  color: #FFFFFF;
`
