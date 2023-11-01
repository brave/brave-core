// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Queries
import {
  useGetSelectedChainQuery //
} from '../../../../../../common/slices/api.slice'

// Components
import { Header } from '../header/header'

// Styled Components
import {
  Background,
  Container,
  ActionButton,
  Wrapper,
  Row
} from './swap-container.style'

interface Props {
  children?: React.ReactNode
  showPrivacyModal: () => void
}

export const SwapContainer = (props: Props) => {
  const { children, showPrivacyModal } = props

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // State
  const [backgroundHeight, setBackgroundHeight] = React.useState<number>(0)
  const [backgroundOpacity, setBackgroundOpacity] = React.useState<number>(0.3)

  // Refs
  const ref = React.createRef<HTMLInputElement>()

  // Methods
  const onClickHelpCenter = React.useCallback(() => {
    window.open(
      'https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet',
      '_blank',
      'noopener'
    )
  }, [])

  // Effects
  React.useEffect(() => {
    // Keeps track of the Swap Containers Height to update
    // the network backgrounds height.
    setBackgroundHeight(ref?.current?.clientHeight ?? 0)
  }, [ref])

  React.useEffect(() => {
    // Changes network background opacity to 0.6 after changing networks
    setBackgroundOpacity(0.6)
    // Changes network background opacity back to 0.3 after 1 second
    setTimeout(() => setBackgroundOpacity(0.3), 1000)
  }, [selectedNetwork])

  return (
    <Wrapper>
      <Header />
      <Container ref={ref}>{children}</Container>
      <Row>
        <ActionButton onClick={showPrivacyModal}>
          {getLocale('braveSwapPrivacyPolicy')}
        </ActionButton>
        <ActionButton onClick={onClickHelpCenter}>
          {getLocale('braveSwapHelpCenter')}
        </ActionButton>
      </Row>
      <Background
        height={backgroundHeight}
        network={selectedNetwork?.chainId ?? ''}
        backgroundOpacity={backgroundOpacity}
      />
    </Wrapper>
  )
}
