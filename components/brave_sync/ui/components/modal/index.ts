/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import Heading from '../../../components/text/heading'

export const ModalHeader = styled<{}, 'header'>('header')`
  margin-bottom: 8px;
`

export const ModalTitle = styled(Heading)`
  font-weight: 500;
  font-size: 24px;
  margin: 0 0 4px;
  line-height: 1.6;
`

interface ModalSubTitleProps {
  highlight?: boolean
}

export const ModalSubTitle = styled<ModalSubTitleProps, 'span'>('span')`
  display: block;
  font-size: 18px;
  line-height: 1.6;
  color: ${p => p.highlight && '#ff0000'}
`

export const ModalContent = styled<{}, 'div'>('div')`
  margin-bottom: -12px;
`

export const OneColumnButtonGrid = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: flex-end;
`

export const TwoColumnButtonGrid = styled<{}, 'footer'>('footer')`
  display: grid;
  align-items: center;
  grid-template-columns: 1fr auto;
  grid-gap: 15px;
  margin-top: 20px;
`

export const ThreeColumnButtonGrid = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: 1fr auto auto;
  grid-gap: 15px;
  margin-top: 20px;
  align-items: center;
`

export const ThreeColumnButtonGridCol1 = styled<{}, 'div'>('div')`
  display: grid;
  align-items: center;
  grid-template-columns: auto;
`

export const ThreeColumnButtonGridCol2 = styled<{}, 'div'>('div')`
  display: grid;
  align-items: center;
  grid-template-columns: auto auto;
  grid-gap: 15px;
`

export const DeviceGrid = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr;
  grid-gap: 30px;
  height: 220px;
  margin: auto;
  align-items: center;
  justify-content: center;
`

export const DeviceContainer = styled<{}, 'a'>('a')`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  border: 1px solid #C8C8D5;
  border-radius: 6px;
  cursor: pointer;
`

export const ScanGrid = styled<{}, 'div'>('div')`
  display: grid;
  height: 100%;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr;
  margin: 25px auto;
  grid-gap: 30px;
  align-items: center;
  width: fit-content;
`

export const QRCodeContainer = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 120px;
  img {
    max-width: 100%;
  }
`

export const ViewSyncCodeGrid = styled<{}, 'div'>('div')`
  display: grid;
  height: 100%;
  grid-template-columns: 2fr 1fr;
  grid-template-rows: 1fr;
  grid-gap: 16px;
  margin: 0 0 25px;
  img {
    max-width: 80%;
  }
`
