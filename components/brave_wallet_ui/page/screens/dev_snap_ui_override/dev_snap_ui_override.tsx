// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { JsonView } from 'react-json-view-lite'
import 'react-json-view-lite/dist/index.css'

// Components
import { boxSampleUi } from '../../../components/snap_ui/box/box_sample'
import { buttonSampleUi } from '../../../components/snap_ui/button/button_sample'
import { headingSampleUi } from '../../../components/snap_ui/heading/heading_sample'
import { SnapUiWalletRenderer } from '../../../components/snap_ui/snap_ui_wallet_renderer'
import type { SnapUiElement } from '../../../components/snap_ui/snap_ui_types'
import { isSnapUiElement } from '../../../components/snap_ui/snap_ui_utils'
import { inputSampleUi } from '../../../components/snap_ui/input/input_sample'
import { textSampleUi } from '../../../components/snap_ui/text/text_sample'

// Styles
import {
  PreviewPanel,
  ButtonEventLog,
  JsonViewPanel,
  SplitPaneColumn,
} from './dev_snap_ui_override.style'
import {
  Column,
  Text,
  Row,
  VerticalDivider,
} from '../../../components/shared/style'

function sampleRoot(sample: unknown): SnapUiElement | null {
  return isSnapUiElement(sample) ? sample : null
}

/**
 * One row: rendered preview + JSON for a single `*_sample` tree. Each instance
 * keeps its own “last button” log so samples do not overwrite each other.
 */
const SampleRow = ({ title, sample }: { title: string; sample: unknown }) => {
  const [lastButtonEvent, setLastButtonEvent] = React.useState<string | null>(
    null,
  )
  const [lastInputEvent, setLastInputEvent] = React.useState<string | null>(
    null,
  )
  const root = sampleRoot(sample)
  const jsonData = (sample ?? {}) as Record<string, unknown>

  return (
    <Column
      fullWidth={true}
      justifyContent='flex-start'
      alignItems='flex-start'
      gap='24px'
    >
      <Text
        textSize='18px'
        textColor='primary'
        textAlign='left'
        isBold={true}
      >
        {title}
      </Text>
      <VerticalDivider />
      <Row
        width='100%'
        justifyContent='flex-start'
        alignItems='flex-start'
        gap='24px'
      >
        <SplitPaneColumn
          gap='12px'
          alignItems='flex-start'
          justifyContent='flex-start'
        >
          {root ? (
            <PreviewPanel
              gap='12px'
              alignItems='flex-start'
              justifyContent='flex-start'
              padding='20px'
            >
              <SnapUiWalletRenderer
                root={root}
                onSnapButton={({ name, snapButtonType }) => {
                  setLastButtonEvent(
                    `name: ${name ?? '(none)'}, type: ${snapButtonType}`,
                  )
                }}
                onSnapInput={({ name, value }) => {
                  setLastInputEvent(
                    `name: ${name}, value: ${JSON.stringify(value)}`,
                  )
                }}
              />
              {lastButtonEvent && (
                <>
                  <Text
                    textSize='14px'
                    textColor='primary'
                    textAlign='left'
                    isBold={true}
                  >
                    Last button
                  </Text>
                  <ButtonEventLog>{lastButtonEvent}</ButtonEventLog>
                </>
              )}
              {lastInputEvent && (
                <>
                  <Text
                    textSize='14px'
                    textColor='primary'
                    textAlign='left'
                    isBold={true}
                  >
                    Last input
                  </Text>
                  <ButtonEventLog>{lastInputEvent}</ButtonEventLog>
                </>
              )}
            </PreviewPanel>
          ) : (
            <Text
              textSize='14px'
              textColor='secondary'
              textAlign='left'
            >
              Invalid sample UI tree.
            </Text>
          )}
        </SplitPaneColumn>

        <SplitPaneColumn
          alignItems='flex-start'
          justifyContent='flex-start'
        >
          <JsonViewPanel>
            <JsonView
              data={jsonData}
              shouldInitiallyExpand={() => true}
            />
          </JsonViewPanel>
        </SplitPaneColumn>
      </Row>
    </Column>
  )
}

export const DevSnapUiOverride = () => {
  return (
    <Column
      fullWidth={true}
      alignItems='flex-start'
      justifyContent='flex-start'
      padding='24px'
      gap='48px'
    >
      <Column
        fullWidth={true}
        justifyContent='flex-start'
        alignItems='flex-start'
        gap='12px'
      >
        <Text
          textSize='18px'
          textColor='primary'
          textAlign='left'
          isBold={true}
        >
          Snaps UI → Brave Wallet UI
        </Text>
        <Column margin='0 0 0 16px'>
          <Text
            textSize='14px'
            textColor='secondary'
            textAlign='left'
          >
            Snaps ship a JSON UI tree (from <code>@metamask/snaps-sdk/jsx</code>
            ), not React. For Brave Wallet we map that tree to Leo / wallet
            components so Snap surfaces match the rest of the product. Each
            section below uses a sample from{' '}
            <code>
              components/snap_ui/&lt;component&gt;/&lt;component&gt;_sample.tsx
            </code>
            . See{' '}
            <a
              href='https://docs.metamask.io/snaps/reference/snaps-api/snap_createinterface/'
              target='_blank'
              rel='noreferrer noopener'
            >
              snap_createInterface
            </a>
            .
          </Text>
        </Column>
      </Column>

      <SampleRow
        title='Box'
        sample={boxSampleUi}
      />
      <SampleRow
        title='Heading'
        sample={headingSampleUi}
      />
      <SampleRow
        title='Text'
        sample={textSampleUi}
      />
      <SampleRow
        title='Input'
        sample={inputSampleUi}
      />
      <SampleRow
        title='Button'
        sample={buttonSampleUi}
      />
    </Column>
  )
}
