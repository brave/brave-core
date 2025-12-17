/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as React from 'react'
import { Light as SyntaxHighlighter } from 'react-syntax-highlighter'
import cpp from 'react-syntax-highlighter/dist/esm/languages/hljs/cpp'
import javascript from 'react-syntax-highlighter/dist/esm/languages/hljs/javascript'
import python from 'react-syntax-highlighter/dist/esm/languages/hljs/python'
import json from 'react-syntax-highlighter/dist/esm/languages/hljs/json'
import { getLocale } from '$web-common/locale'
import useMediaQuery from '$web-common/useMediaQuery'
import darkStyle from './dark_style'
import lightStyle from './light_style'
import styles from './style.module.scss'

SyntaxHighlighter.registerLanguage('cpp', cpp)
SyntaxHighlighter.registerLanguage('javascript', javascript)
SyntaxHighlighter.registerLanguage('python', python)
SyntaxHighlighter.registerLanguage('json', json)

interface CodeInlineProps {
  code: string
}
interface CodeBlockProps {
  code: string
  lang: string
}

function Inline(props: CodeInlineProps) {
  return (
    <span className={styles.container}>
      <code>{props.code}</code>
    </span>
  )
}

function Block(props: CodeBlockProps) {
  const isDarkMode = useMediaQuery('(prefers-color-scheme: dark)')
  const [hasCopied, setHasCopied] = React.useState(false)

  const handleCopy = () => {
    navigator.clipboard.writeText(props.code).then(() => {
      setHasCopied(true)
      setTimeout(() => setHasCopied(false), 1000)
    })
  }

  return (
    <div className={styles.container}>
      <div className={styles.toolbar}>
        <div>{props.lang}</div>
        <Button
          kind='plain-faint'
          onClick={handleCopy}
        >
          <div slot='icon-before'>
            <Icon
              className={styles.icon}
              name={hasCopied ? 'check-circle-outline' : 'copy'}
            />
          </div>
          <div>{getLocale(S.CHAT_UI_COPY_BUTTON_LABEL)}</div>
        </Button>
      </div>
      <SyntaxHighlighter
        language={props.lang}
        style={isDarkMode ? darkStyle : lightStyle}
        wrapLines
        wrapLongLines
        codeTagProps={{ style: { wordBreak: 'break-word' } }}
      >
        {props.code}
      </SyntaxHighlighter>
    </div>
  )
}

export default {
  Inline,
  Block,
}
