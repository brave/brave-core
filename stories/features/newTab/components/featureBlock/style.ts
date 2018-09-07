/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { FeatureBlockProps } from './index'

export const StyledFeatureBlock = styled<{}, 'div'>('div')`
  display: grid;
  height: 100%;
  grid-gap: ${(p: FeatureBlockProps) => p.grid ? '30px' : '0'};
  grid-template-columns: ${(p: FeatureBlockProps) => p.grid ? '1fr 5fr 1fr' : '1fr'};
  grid-template-rows: 1fr;
  max-width: 800px;
  border-top: solid 1px rgba(255,255,255,.1);
  padding: 30px 0 30px;

  aside {
    display: flex;
    flex-direction: column;
    justify-content: center;
  }
`
