// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import classNames from '$web-common/classnames'
import styles from './input.module.scss'
import '../app.global.scss'

type CustomProps = {
  label?: string
  errorMessage?: string
  isRequired?: boolean
  isErrorAlwaysShown?: boolean
  value?: string | boolean | readonly string[]
}

type Props = CustomProps & React.HTMLProps<HTMLInputElement>

function Error (props: CustomProps) {
  let errorMessage: string = ''
  if (props.isRequired && props.isErrorAlwaysShown && !props.value) {
    // TODO: use i18n key and make sure all consuming webuis add that key to the strings
    errorMessage = 'This field is required'
  }
  if (props.errorMessage) {
    errorMessage = props.errorMessage
  }
  if (!errorMessage) {
    return null
  }
  return <span data-has-error className={styles.errorMessage}>{errorMessage}</span>
}

export default function TextInput (props: Props) {
  const { errorMessage, label, isRequired, isErrorAlwaysShown, ...inputProps } = props
  return (
    <label className={classNames(styles.textInput, errorMessage ? styles.hasError : undefined)}>
      {label}
      <input
        type="text"
        {...inputProps}
      />
      <Error {...props} />
    </label>
  )
}

export function Textarea (props: CustomProps & React.HTMLProps<HTMLTextAreaElement>) {
  const { errorMessage, label, isRequired, isErrorAlwaysShown, ...textareaprops } = props
  return (
    <label className={classNames(styles.textInput, errorMessage ? styles.hasError : undefined)}>
      {label}
      <textarea
        {...textareaprops}
      ></textarea>
      <Error {...props} />
    </label>
  )
}
