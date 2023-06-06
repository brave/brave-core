// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv, StyledButton } from '../../shared-swap.styles'

export const Wrapper = styled(StyledDiv)`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 68px 0px;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  overflow-y: scroll;
  position: absolute;
  background-color: ${p => p.theme.color.background01};
  @media (prefers-color-scheme: dark) {
    background-color: ${p => p.theme.color.background02};
  }
  @media screen and (max-width: 570px) {
    padding: 68px 0px;
  }
`

export const Background = styled(StyledDiv) <{
  height: number
  network: string
  backgroundOpacity: number
}>`
  /* Solana */
  --0x65: linear-gradient(
    125deg,
    rgb(33, 178, 164) 0%,
    rgb(93, 124, 209) 50%,
    rgb(122, 96, 232) 100%
  );
  /* Ethereum */
  --0x1: linear-gradient(125deg, rgb(98, 126, 234) 0%, rgb(129, 152, 238) 100%);
  /* Polygon */
  --0x89: linear-gradient(
    125deg,
    rgb(130, 71, 229) 0%,
    rgb(93, 124, 209) 50%,
    rgb(130, 71, 229) 100%
  );
  /* Avalanche */
  --0xa86a: linear-gradient(
    125deg,
    rgb(232, 65, 66) 0%,
    rgb(233, 175, 176) 50%,
    rgb(232, 65, 66) 100%
  );
  /* Optimism */
  --0xa: linear-gradient(
    125deg,
    rgb(252, 141, 153) 0%,
    rgb(247, 211, 215) 50%,
    rgb(254, 4, 32) 100%
  );
  /* Celo */
  --0xa4ec: linear-gradient(
    125deg,
    rgb(252, 204, 94) 0%,
    rgb(238, 255, 143) 50%,
    rgb(54, 210, 129) 100%
  );
  /* Binance */
  --0x38: linear-gradient(
    125deg,
    rgb(243, 186, 47) 0%,
    rgb(255, 219, 133) 50%,
    rgb(243, 186, 47) 100%
  );
  /* Fantom */
  --0xfa: linear-gradient(
    125deg,
    rgb(19, 181, 236) 0%,
    rgb(19, 181, 236) 50%,
    rgb(19, 181, 236) 100%
  );
  filter: blur(150px);
  width: 512px;
  height: ${p => p.height}px;
  opacity: ${p => p.backgroundOpacity};
  transition-delay: 0s;
  transition-duration: 1s;
  transition-timing-function: ease;
  position: absolute;
  z-index: 8;
  background-image: var(--${p => p.network});
  @media screen and (max-width: 570px) {
    width: 100%;
  }
`

export const Container = styled(StyledDiv)`
  background-color: ${p => p.theme.color.background01};
  border-radius: 24px;
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  justify-content: flex-start;
  padding: 16px;
  width: 512px;
  position: relative;
  z-index: 9;
  margin-bottom: 10px;
  @media screen and (max-width: 570px) {
    width: 92%;
    padding: 16px 8px;
  }
`

export const Row = styled.div`
  display: flex;
  flex-direction: row;
  gap: 5px;
`

export const ActionButton = styled(StyledButton)`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.color.interactive06};
  }
`
