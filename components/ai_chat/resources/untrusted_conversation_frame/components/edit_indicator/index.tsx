/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Time } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
import { mojoTimeToJSDate } from '$web-common/mojomUtils'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'

import styles from './style.module.scss'

interface Props {
  time: Time;
}

const dateTimeFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric',
  year: 'numeric',
  hour: 'numeric',
  minute: 'numeric'
});

function EditIndicator(props: Props) {
  return (
    <div className={styles.editIndicator}>
      <Icon name='edit-pencil' />
      <span className={styles.editedText}>{getLocale('editedLabel')}</span>
      <span className={styles.time}>
        {dateTimeFormatter.format(mojoTimeToJSDate(props.time))}
      </span>
    </div>
  );
}

export default EditIndicator;
