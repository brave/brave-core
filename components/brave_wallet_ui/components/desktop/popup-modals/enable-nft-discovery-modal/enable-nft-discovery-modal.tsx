// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// selectors
import {
  useSafeUISelector //
} from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'
import { loadTimeData } from '../../../../../common/loadTimeData'

// components
import { PopupModal } from '../../popup-modals/index'

// styles
import {
  ButtonRow,
  Description,
  Header,
  Link,
  Underline
} from './enable-nft-discovery-modal.style'
import { LeoSquaredButton, Column } from '../../../shared/style'

interface Props {
  onConfirm: () => void
  onCancel: () => void
}

const LEARN_MORE_LINK =
  'https://github.com/brave/brave-browser/wiki/NFT-Discovery'

export const EnableNftDiscoveryModal = ({ onConfirm, onCancel }: Props) => {
  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = loadTimeData.getBoolean('isAndroid') || false

  const { beforeTag, duringTag, afterTag } = splitStringForTag(
    getLocale('braveWalletEnableNftAutoDiscoveryModalDescription'),
    1
  )
  const { beforeTag: beforeLink, duringTag: learnMore } = splitStringForTag(
    afterTag || '',
    3
  )

  return (
    <PopupModal
      title=''
      width='477px'
      hideHeader={!isPanel && !isAndroid}
      onClose={onCancel}
    >
      <Column
        fullHeight={true}
        fullWidth={true}
        justifyContent='flex-start'
        padding='32px 40px'
      >
        <Header>
          {getLocale('braveWalletEnableNftAutoDiscoveryModalHeader')}
        </Header>
        <Description>
          {beforeTag}
          <Underline>{duringTag}</Underline>
          {beforeLink}
          <Link
            target='_blank'
            rel='noreferrer'
            href={LEARN_MORE_LINK}
          >
            {learnMore}
          </Link>
        </Description>
        <ButtonRow>
          <LeoSquaredButton
            onClick={onCancel}
            kind='plain'
          >
            {getLocale('braveWalletEnableNftAutoDiscoveryModalCancel')}
          </LeoSquaredButton>
          <LeoSquaredButton onClick={onConfirm}>
            {getLocale('braveWalletEnableNftAutoDiscoveryModalConfirm')}
          </LeoSquaredButton>
        </ButtonRow>
      </Column>
    </PopupModal>
  )
}
