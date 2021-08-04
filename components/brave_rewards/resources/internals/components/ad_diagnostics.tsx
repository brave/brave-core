/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Button } from 'brave-ui/components'
import { ButtonWrapper, DiagnosticsEntry } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  entries: RewardsInternals.AdDiagnosticsEntry[]
  onGet: () => void
}

const getEntries = (entries: RewardsInternals.AdDiagnosticsEntry[]) => {
  if (!entries || entries.length === 0) {
    return (
      <DiagnosticsEntry>
        {getLocale('adsNotInitialized')}
      </DiagnosticsEntry>
    )
  }

  return entries.map(entry => (
    <DiagnosticsEntry key={entry.key}>
      {entry.key}: {entry.value}
    </DiagnosticsEntry>
  ))
}

export const AdDiagnostics = (props: Props) => {
  return (
    <>
      <ButtonWrapper>
        <Button
          text={getLocale('refreshButton')}
          size={'medium'}
          type={'accent'}
          onClick={props.onGet}
        />
      </ButtonWrapper>
      {getEntries(props.entries)}
    </>
  )
}
