// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/nala/react/alert'
import Button from '@brave/nala/react/button'
import Collapse from '@brave/nala/react/collapse'
import Icon from '@brave/nala/react/icon'
import Input from '@brave/nala/react/input'
import { isCancelledError } from '$web-common/api'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { ConnectorStatusMessage, ConnectorToolCount } from './connector_status'
import {
  ConnectorField,
  getConnectorErrorField,
  getConnectorErrorMessage,
  getConnectorOrigin,
  getMCPServerError,
  parseConnectorUrl,
} from './connector_utils'
import styles from './style.module.scss'

interface Props {
  /** The connector to edit, or undefined to add one. */
  server?: Mojom.MCPServer
  onCancel: () => void
  onSaved: (server: Mojom.MCPServer) => void
  /**
   * The dialog mustn't be left while a save is in progress, as the steps after
   * the update only run while the editor is open.
   */
  onSavingChange: (isSaving: boolean) => void
}

type TestResult = { tools: Mojom.MCPServerTool[] } | { error: unknown }

type FieldErrors = Partial<Record<ConnectorField, string>>

// Fields in the order they're shown, so that the first error gets focus.
const kFields: ConnectorField[] = ['name', 'url', 'token']

function focusSoon(element: React.RefObject<HTMLElement | null>) {
  // After the render that shows the element, or its error.
  setTimeout(() => element.current?.focus())
}

/**
 * Adds a connector, or edits one. Its token is write-only: the page only ever
 * knows whether there is one, and whether it can be read.
 */
export default function ConnectorEditor(props: Props) {
  const { api } = useAIChat()
  const { server, onSavingChange } = props

  const [name, setName] = React.useState(server?.name ?? '')
  const [url, setUrl] = React.useState(server?.endpoint.url ?? '')
  const [token, setToken] = React.useState('')
  const [isReplacingToken, setIsReplacingToken] = React.useState(false)
  // Applied on Save, so that Cancel keeps the token.
  const [isTokenRemoved, setIsTokenRemoved] = React.useState(false)
  const [isAuthenticationOpen, setIsAuthenticationOpen] = React.useState(
    !!server?.hasToken,
  )
  const [fieldErrors, setFieldErrors] = React.useState<FieldErrors>({})
  const [formError, setFormError] = React.useState<string>()
  const [isTesting, setIsTesting] = React.useState(false)
  const [testResult, setTestResult] = React.useState<TestResult>()

  const nameRef = React.useRef<HTMLElement>(null)
  const urlRef = React.useRef<HTMLElement>(null)
  const tokenRef = React.useRef<HTMLElement>(null)
  const fieldRefs = { name: nameRef, url: urlRef, token: tokenRef }

  // Their per-call callbacks don't run once the editor is gone, so a save
  // that finishes after the dialog closes doesn't reopen it.
  const addServer = api.useAddServer()
  const updateServer = api.useUpdateServer()
  const clearServerToken = api.useClearServerToken()
  const [isSaving, setIsSaving] = React.useState(false)
  // Set as soon as a save starts, as a second Enter can come before the
  // render that shows the first.
  const isSaveStarted = React.useRef(false)

  React.useEffect(() => {
    onSavingChange(isSaving)
    return () => onSavingChange(false)
  }, [isSaving, onSavingChange])

  React.useEffect(() => {
    focusSoon(nameRef)
  }, [])

  const hasToken = !!server?.hasToken && !isTokenRemoved
  const hasReadableToken = hasToken && !server.tokenUnreadable
  const showTokenInput = !hasReadableToken || isReplacingToken
  const newToken = showTokenInput && token.trim() ? token : null

  // The registry drops a token when its endpoint moves to another origin.
  const newOrigin = getConnectorOrigin(url.trim())
  const isTokenDropped =
    hasToken
    && newToken === null
    && !!newOrigin
    && newOrigin !== getConnectorOrigin(server.endpoint.url)

  // A test uses the saved settings, so would ignore any unsaved changes.
  const hasChanges =
    !!server
    && (name !== server.name
      || url !== server.endpoint.url
      || !!newToken
      || isTokenRemoved)

  const changeField = (field: ConnectorField, set: (value: string) => void) => {
    return ({ value }: { value: string }) => {
      set(value)
      setTestResult(undefined)
      setFieldErrors(({ [field]: _, ...rest }) => rest)
    }
  }

  const showErrors = (errors: FieldErrors) => {
    setFieldErrors(errors)
    const field = kFields.find((f) => errors[f])
    if (!field) {
      return
    }
    if (field === 'token') {
      setIsAuthenticationOpen(true)
    }
    focusSoon(fieldRefs[field])
  }

  const onSaveError = (error: unknown) => {
    isSaveStarted.current = false
    setIsSaving(false)
    const message = getConnectorErrorMessage(error)
    const field = getConnectorErrorField(getMCPServerError(error))
    if (field) {
      showErrors({ [field]: message })
    } else {
      setFormError(message)
    }
  }

  const save = () => {
    if (isSaveStarted.current) {
      return
    }
    setFormError(undefined)
    const errors: FieldErrors = {}
    if (!name.trim()) {
      errors.name = getConnectorErrorMessage(Mojom.MCPServerError.kInvalidName)
    }
    const endpoint = parseConnectorUrl(url)
    if ('error' in endpoint) {
      errors.url = getConnectorErrorMessage(endpoint.error)
    }
    showErrors(errors)
    if ('error' in endpoint || errors.name) {
      return
    }

    const params: Mojom.MCPServerParams = {
      name: name.trim(),
      endpoint: { url: endpoint.url },
      enabled: server?.enabled ?? true,
      autoAttach: server?.autoAttach ?? false,
    }
    isSaveStarted.current = true
    setIsSaving(true)
    if (!server) {
      addServer.mutate([params, newToken], {
        onSuccess: (saved) => props.onSaved(saved),
        onError: onSaveError,
      })
      return
    }
    updateServer.mutate([server.id, params, newToken], {
      onSuccess: (saved) => {
        // Removed only once the update is accepted, so that a setting the
        // host rejects leaves the token as it was.
        if (!isTokenRemoved || newToken !== null || !saved.hasToken) {
          props.onSaved(saved)
          return
        }
        clearServerToken.mutate([server.id], {
          onSuccess: (cleared) => props.onSaved(cleared),
          onError: onSaveError,
        })
      },
      onError: onSaveError,
    })
  }

  const removeToken = () => {
    setIsTokenRemoved(true)
    setIsReplacingToken(false)
    setToken('')
    setTestResult(undefined)
    setFieldErrors(({ token: _, ...rest }) => rest)
    focusSoon(tokenRef)
  }

  const test = async () => {
    if (!server) {
      return
    }
    setIsTesting(true)
    setTestResult(undefined)
    // Asks the host again, rather than showing what it last answered.
    api.getServerTools.reset(server.id)
    let result: TestResult
    try {
      result = { tools: await api.getServerTools.fetch(server.id) }
    } catch (error) {
      // A fetch is cancelled when the connector changes.
      result = {
        error: isCancelledError(error)
          ? Mojom.MCPServerError.kServerChanged
          : error,
      }
    }
    setTestResult(result)
    setIsTesting(false)
  }

  const saveOnEnter = (detail: { innerEvent: Event }) => {
    if ((detail.innerEvent as KeyboardEvent).key === 'Enter') {
      save()
    }
  }

  return (
    <>
      <div className={styles.body}>
        <div className={styles.description}>
          {getLocale(S.CHAT_UI_CONNECTOR_EDITOR_DESCRIPTION)}
        </div>
        {formError && <Alert type='error'>{formError}</Alert>}
        <div className={styles.form}>
          <Input
            ref={nameRef}
            value={name}
            onInput={changeField('name', setName)}
            onKeyDown={saveOnEnter}
            showErrors={!!fieldErrors.name}
            data-testid='connector-name'
          >
            {getLocale(S.CHAT_UI_CONNECTOR_NAME_LABEL)}
            <div slot='errors'>{fieldErrors.name}</div>
          </Input>
          <div className={styles.field}>
            <Input
              ref={urlRef}
              type='url'
              value={url}
              placeholder={getLocale(S.CHAT_UI_CONNECTOR_URL_PLACEHOLDER)}
              onInput={changeField('url', setUrl)}
              onKeyDown={saveOnEnter}
              showErrors={!!fieldErrors.url}
              data-testid='connector-url'
            >
              {getLocale(S.CHAT_UI_CONNECTOR_URL_LABEL)}
              <div slot='errors'>{fieldErrors.url}</div>
            </Input>
            {isTokenDropped && (
              <ConnectorStatusMessage
                kind='warning'
                text={getLocale(S.CHAT_UI_CONNECTOR_TOKEN_ORIGIN_CHANGED)}
              />
            )}
          </div>
          <Collapse
            isOpen={isAuthenticationOpen}
            onToggle={({ open }) => setIsAuthenticationOpen(open)}
          >
            <div slot='title'>
              {getLocale(S.CHAT_UI_CONNECTOR_AUTHENTICATION_LABEL)}
            </div>
            <div className={styles.authentication}>
              <div className={styles.description}>
                {getLocale(S.CHAT_UI_CONNECTOR_TOKEN_DESCRIPTION)}
              </div>
              {hasToken && server.tokenUnreadable && (
                <ConnectorStatusMessage
                  kind='warning'
                  text={getConnectorErrorMessage(
                    Mojom.MCPServerError.kTokenUnreadable,
                  )}
                />
              )}
              {showTokenInput ? (
                <Input
                  ref={tokenRef}
                  type='password'
                  value={token}
                  onInput={changeField('token', setToken)}
                  onKeyDown={saveOnEnter}
                  showErrors={!!fieldErrors.token}
                  data-testid='connector-token'
                >
                  {getLocale(S.CHAT_UI_CONNECTOR_TOKEN_LABEL)}
                  <div slot='errors'>{fieldErrors.token}</div>
                </Input>
              ) : (
                <div className={styles.savedToken}>
                  <Icon name='key' />
                  {getLocale(S.CHAT_UI_CONNECTOR_TOKEN_SAVED)}
                </div>
              )}
              {isTokenRemoved && newToken === null && (
                <div className={styles.status}>
                  {getLocale(S.CHAT_UI_CONNECTOR_TOKEN_REMOVED_ON_SAVE)}
                </div>
              )}
              {hasToken && (
                <div className={styles.tokenActions}>
                  {hasReadableToken
                    && (isReplacingToken ? (
                      <Button
                        kind='plain-faint'
                        onClick={() => {
                          setIsReplacingToken(false)
                          setToken('')
                          setFieldErrors(({ token: _, ...rest }) => rest)
                        }}
                      >
                        {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
                      </Button>
                    ) : (
                      <Button
                        kind='outline'
                        onClick={() => {
                          setIsReplacingToken(true)
                          focusSoon(tokenRef)
                        }}
                      >
                        {getLocale(
                          S.CHAT_UI_CONNECTOR_TOKEN_REPLACE_BUTTON_LABEL,
                        )}
                      </Button>
                    ))}
                  <Button
                    kind='plain-faint'
                    onClick={removeToken}
                  >
                    {getLocale(S.CHAT_UI_CONNECTOR_TOKEN_REMOVE_BUTTON_LABEL)}
                  </Button>
                </div>
              )}
            </div>
          </Collapse>
          {server && (
            <div className={styles.test}>
              <Button
                kind='outline'
                isDisabled={hasChanges || isSaving}
                isLoading={isTesting}
                onClick={test}
              >
                {getLocale(S.CHAT_UI_CONNECTOR_TEST_BUTTON_LABEL)}
              </Button>
              {testResult
                && ('tools' in testResult ? (
                  <ConnectorToolCount
                    count={testResult.tools.length}
                    isTestResult
                  />
                ) : (
                  <ConnectorStatusMessage
                    kind='warning'
                    text={getConnectorErrorMessage(testResult.error)}
                  />
                ))}
            </div>
          )}
        </div>
      </div>
      <div slot='actions'>
        <Button
          kind='plain-faint'
          isDisabled={isSaving}
          onClick={props.onCancel}
        >
          {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
        </Button>
        <Button
          kind='filled'
          isLoading={isSaving}
          onClick={save}
        >
          {getLocale(S.CHAT_UI_SAVE_BUTTON_LABEL)}
        </Button>
      </div>
    </>
  )
}
