/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components group
import {
  ModalLayout,
  SuccessIcon,
  IconTitle,
  ModalTitle,
  TextSection,
  InfoText
} from '../../../components/basic'

// Fake data
import { getLocale } from '../fakeLocale'

export default class ConfirmationView extends React.PureComponent<{}, {}> {
  constructor (props: {}) {
    super(props)
  }

  render () {
    return (
      <ModalLayout>
        <IconTitle>
          <SuccessIcon />
          <ModalTitle>{getLocale('thankYou')}</ModalTitle>
        </IconTitle>
        <TextSection>
          <InfoText>{getLocale('confirmationNotice')}</InfoText>
        </TextSection>
      </ModalLayout>
    )
  }
}
