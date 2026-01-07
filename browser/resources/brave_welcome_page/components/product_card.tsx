/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'

import { getString } from '../lib/strings'

import { style } from './product_card.style'

interface Props {
  image: string
  title: string
  description: string
  learnMoreUrl?: string
  checked: boolean
  onChange: (checked: boolean) => void
}

export function ProductCard(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <img src={props.image} />
      <div className='text'>
        <h3>{props.title}</h3>
        <p>
          {props.description}
          {props.learnMoreUrl && (
            <>
              {' '}
              <a
                href={props.learnMoreUrl}
                target='_blank'
                rel='noopener noreferrer'
              >
                {getString('WELCOME_PAGE_LEARN_MORE_LINK_LABEL')}
              </a>
            </>
          )}
        </p>
      </div>
      <Checkbox
        checked={props.checked}
        onChange={(event) => props.onChange(event.checked)}
      />
    </div>
  )
}
