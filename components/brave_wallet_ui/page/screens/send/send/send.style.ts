// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Assets
import { LoaderIcon } from 'brave-ui/components/icons'

// Shared Styles
import { StyledDiv, StyledInput, Row } from '../shared.styles'

export const SendContainer = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 24px;
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  justify-content: flex-start;
  padding: 16px;
  width: 512px;
  position: relative;
  z-index: 9;
  @media screen and (max-width: 570px) {
    width: 90%;
  }
`

export const SectionBox = styled(StyledDiv) <{
  hasError?: boolean
  hasWarning?: boolean
  minHeight?: number
  noPadding?: boolean
  boxDirection?: 'row' | 'column'
}>`
  background-color: ${(p) => p.theme.color.background02};
  flex-direction: ${(p) => p.boxDirection ? p.boxDirection : 'column'};
  box-sizing: border-box;
  border-radius: 16px;
  border: 1px solid
    ${(p) => (p.hasError ? p.theme.color.errorBorder : p.hasWarning ? p.theme.color.warningBorder : p.theme.color.divider01)};
  padding: ${(p) => p.noPadding ? '0px' : '16px 16px 16px 8px'};
  width: 100%;
  position: relative;
  margin-bottom: 16px;
  min-height: ${(p) => (p.minHeight ? `${p.minHeight}px` : 'unset')};
`

export const AmountInput = styled(StyledInput) <{
  hasError: boolean
}>`
  color: ${(p) => (p.hasError ? p.theme.color.errorBorder : 'inherit')};
  font-weight: 500;
  font-size: 28px;
  line-height: 42px;
  text-align: right;
  width: 100%;
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
  }
`

export const AddressInput = styled(StyledInput) <{
  hasError: boolean
}>`
  color: ${(p) => (p.hasError ? p.theme.color.errorBorder : 'inherit')};
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
  width: 100%;
  z-index: 9;
  position: relative;
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
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
  --0x66: var(--0x65);
  --0x67: var(--0x65);
  --0x539501: var(--0x65);
  /* Ethereum */
  --0x1: linear-gradient(125deg, rgb(98, 126, 234) 0%, rgb(129, 152, 238) 100%);
  --0x5: var(--0x1);
  --0xaa36a7: var(--0x1);
  --0x53960: var(--0x1);
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
  /* Aurora */
  --0x4e454152: linear-gradient(
    125deg,
    rgb(54, 210, 129) 0%,
    rgb(54, 210, 129) 50%,
    rgb(54, 210, 129) 100%
  );
  /* Filecoin */
  --f: linear-gradient(
    125deg,
    rgb(19, 181, 236) 0%,
    rgb(19, 181, 236) 50%,
    rgb(19, 181, 236) 100%
  );
  --t: var(--f);
  --0x539461: var(--f);
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
`

export const DIVForWidth = styled.div`
  width: auto;
  display: inline-block;
  visibility: hidden;
  position: fixed;
  overflow:auto;
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
`

export const InputRow = styled(Row)`
  box-sizing: border-box;
  position: relative;
`

export const DomainLoadIcon = styled(LoaderIcon) <{ position: number }>`
  color: ${p => p.theme.palette.blurple500};
  height: 16px;
  width: 16px;
  opacity: 0.4;
  position: absolute;
  z-index: 8;
  left: ${(p) => p.position}px;
`
