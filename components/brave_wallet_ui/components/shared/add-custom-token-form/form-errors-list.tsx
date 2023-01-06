// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Column } from '../style'
import { ErrorText } from './add-custom-token-form-styles'

interface Props {
  errors: Array<false | string>
}

export const FormErrorsList = ({ errors }: Props) => {
  return (
    <Column
      padding={8}
      fullWidth
      alignItems={'flex-start'}
      justifyContent='flex-start'
    >
      {errors.map((err) =>
        err ? <ErrorText key={err}>{err}</ErrorText> : null
      )}
    </Column>
  )
}
