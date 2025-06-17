// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from "@brave/leo/react/button";
import ProgressRing from '@brave/leo/react/progressRing';
import classnames from '$web-common/classnames';
import styles from './style.module.scss'

interface Props {
  onClick: () => void
  className?: string
  isLoading?: boolean
  icon?: React.JSX.Element
  isDisabled?: boolean
}

export default function ConversationAreaButton(props: React.PropsWithChildren<Props>) {
  // TODO(petemill): Don't use Nala Button since our styles have
  // diverged and it's becoming difficult to position this content
  // full-width as well as use some features of the Button, like
  // the loading spinner.
  return <Button
    kind='outline'
    size='small'
    onClick={props.onClick}
    isDisabled={!!props.isDisabled}
    isLoading={props.isLoading}
    className={classnames(styles.suggestion, props.className)}>
    <div className={styles.container}>
      {props.icon &&
      <div className={styles.icon}>
          {props.icon}
      </div>
      }
      <span className={styles.buttonText}>{props.children}</span>
      {props.isLoading && <ProgressRing />}
    </div>
  </Button>
}

