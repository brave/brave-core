// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client';
import Button from '@brave/leo/react/button'
import { color, spacing } from '@brave/leo/tokens/css/variables'
import { Alias, MappingService, ViewMode } from './types'
import Icon from '@brave/leo/react/icon'
import { StyleSheetManager } from 'styled-components'
import Input, { InputEventDetail } from '@brave/leo/react/input'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Tooltip from '@brave/leo/react/tooltip'
import { getLocale } from '$web-common/locale'
import formatMessage from '$web-common/formatMessage'
import * as S from './styles'

const MAX_ALIASES = 5;

type ViewState = {
  mode: ViewMode,
  alias?: Alias
}

const onEnterKey = (onSubmit: Function) =>
  (e: InputEventDetail) => {
    const innerEvent = e.innerEvent as unknown as KeyboardEvent
    if (innerEvent.key === 'Enter') {
      onSubmit()
    }
  }

const BraveIcon = ({style}: {style?: React.CSSProperties}) => (
  <S.BraveIconCircle style={{...style, flexGrow: 0}}>
    <S.BraveIconWrapper>
      <Icon name='brave-icon-release-color' />
    </S.BraveIconWrapper>
  </S.BraveIconCircle>
)

const Introduction = () => (
 <S.Card id='introduction'>
    <h2>{getLocale('emailAliasesShortDescription')}</h2>
    <div>{getLocale('emailAliasesDescription')} <a href="https://support.brave.com" target='_blank'>{getLocale('emailAliasesLearnMore')}</a>
    </div>
  </S.Card>
)

const MainEmailDisplay = ({ email, onLogout }: { email: string, onLogout: Function }) => (
  <S.Card id='main-email-display'>
    <S.AccountRow>
    <S.Row>
      <BraveIcon />
      <S.MainEmailTextContainer>
        <S.MainEmail>{email === '' ? getLocale('emailAliasesConnectingToBraveAccount') : email}</S.MainEmail>
        <S.MainEmailDescription>{getLocale('emailAliasesBraveAccount')}</S.MainEmailDescription>
      </S.MainEmailTextContainer>
    </S.Row>
    <S.ManageAccountLink
      title={getLocale('emailAliasesSignOutTitle')}
      href='#'
      onClick={(e) => { e.preventDefault(); onLogout() }}>
      <Icon name="outside" />
      <span style={{ margin: '0.25em' }}>{getLocale('emailAliasesSignOut')}</span>
      </S.ManageAccountLink>
  </S.AccountRow>
</S.Card>
)

const copyEmailToClipboard = (
  email: string
) => {
  navigator.clipboard.writeText(email)
}

const AliasMenuItem = ({ onClick, iconName, text }:
  { onClick: EventListener, iconName: string, text: string }) => (
  <leo-menu-item
    onClick={onClick}>
    <S.Row style={{ fontSize: '90%' }}>
      <Icon name={iconName} />
      <span style={{ marginInlineStart: '0.5em' }}>{text}</span>
    </S.Row>
  </leo-menu-item>
)

const CopyToast = ({ children }: React.PropsWithChildren) => {
  const [copied, setCopied] = React.useState<boolean>(false)
  return (<div onClick={() => {
    setCopied(true)
    setTimeout(() => setCopied(false), 1000)
  }}>
    <Tooltip text={copied ? getLocale('emailAliasesCopiedToClipboard') : ''} mode="mini" visible={copied}>
      {children}
    </Tooltip>
  </div>
  )
}

const AliasItem = ({ alias, onEdit, onDelete }: { alias: Alias, onEdit: Function, onDelete: Function }) => {
  return (
    <S.AliasItemRow>
      <S.Col>
        <CopyToast>
          <S.EmailContainer title={getLocale('emailAliasesClickToCopyAlias')}
            onClick={(event: React.MouseEvent<HTMLElement>) => copyEmailToClipboard(alias.email)}>
            {alias.email}
          </S.EmailContainer>
        </CopyToast>
        {((alias.note || alias.domains) &&
          <S.AliasAnnotation>
            {(alias.note && <span>{alias.note}</span>)}
            {alias.domains && alias.note && <span>. </span>}
            {(alias.domains && <span>{formatMessage( getLocale('emailAliasesUsedBy'),
              { placeholders: { $1: alias.domains?.join(", ") } })}</span>)}
          </S.AliasAnnotation>
        )}
      </S.Col>
      <S.AliasControls>
        <CopyToast>
          <S.CopyButtonWrapper
            title={getLocale('emailAliasesClickToCopyAlias')}
            onClick={() => {
              copyEmailToClipboard(alias.email)
            }}>
            <Icon name="copy" style={{color: color.text.secondary}}/>
          </S.CopyButtonWrapper>
        </CopyToast>
        <ButtonMenu>
          <S.MenuButton slot='anchor-content' kind='plain-faint' size="large" style='width: 1.5em;'>
            <Icon name="more-vertical" />
          </S.MenuButton>
          <AliasMenuItem
            iconName="edit-pencil"
            text={getLocale('emailAliasesEdit')}
            onClick={() => onEdit()} />
          <AliasMenuItem
            iconName="trash"
            text={getLocale('emailAliasesDelete')}
            onClick={() => onDelete(alias)} />
        </ButtonMenu>
      </S.AliasControls>
    </S.AliasItemRow>
  )
}

const AliasList = ({ aliases, onViewChange, onListChange, mappingService }: { mappingService: MappingService, aliases: Alias[], onViewChange: Function, onListChange: Function }) => (
  <S.Card style={{ borderTop: `1px solid ${color.legacy.divider1}` }}>
    <S.AliasListIntro>
      <S.Col>
        <h2>{getLocale('emailAliasesListTitle')}</h2>
        <div>
          {getLocale('emailAliasesCreateDescription')}
        </div>
      </S.Col>
      <Button style='flex-grow: 0;'
        isDisabled={aliases.length >= MAX_ALIASES}
        title={getLocale('emailAliasesCreateAliasTitle')}
        id='add-alias'
        onClick={
          async () => {
            onViewChange({ mode: ViewMode.Create })
            const newEmailAlias = await mappingService.generateAlias()
            onViewChange({ mode: ViewMode.Create, alias: { email: newEmailAlias } })
          }
        }>
        {getLocale('emailAliasesCreateAliasLabel')}
      </Button>
    </S.AliasListIntro>
    {aliases.map(
      alias => <AliasItem
        key={alias.email}
        alias={alias}
        onEdit={() => onViewChange({ mode: ViewMode.Edit, alias: alias })}
        onDelete={async (alias: Alias) => {
          await mappingService.deleteAlias(alias.email)
          onListChange()
        }}></AliasItem>)}
  </S.Card>
)

const RefreshButton = ( { onClicked } : { onClicked: Function }) => (
  <Button title={getLocale('emailAliasesRefreshButtonTitle')}
    onClick={() => onClicked()}
    kind="plain" style='flex-grow: 0; padding: 0px'>
    <Icon name="refresh" />
  </Button>
)

const ModalWithCloseButton = ({ children, returnToMain }: React.PropsWithChildren & { returnToMain: Function }) => (
  <S.Modal>
    <S.CloseButton onClick={() => returnToMain()}><Icon name='close' /></S.CloseButton>
    {children}
  </S.Modal>
)

export const EmailAliasModal = (
  { returnToMain, viewState, email, mode, mappingService, bubble }:
    { returnToMain: Function,
      viewState?: ViewState,
      bubble?: boolean,
      mode: ViewMode
      email: string,
      mappingService: MappingService }
) => {
  const [limitReached, setLimitReached] = React.useState<boolean>(false)
  const [mainEmail, setMainEmail] = React.useState<string>(email)
  const [proposedAlias, setProposedAlias] = React.useState<string>(viewState?.alias?.email ?? '')
  const [proposedNote, setProposedNote] = React.useState<string>(viewState?.alias?.note ?? '')
 // const noteInputRef = React.useRef<HTMLInputElement>(null)
  const createOrSave = async () => {
    if (proposedAlias !== '') {
      if (mode === ViewMode.Create) {
        await mappingService.createAlias(proposedAlias, proposedNote)
        await mappingService.fillField(proposedAlias)
      } else {
        await mappingService.updateAlias(proposedAlias, proposedNote, true)
      }
      returnToMain()
    }
  }
  const regenerateAlias = async () => {
    const newEmailAlias = await mappingService.generateAlias()
    setProposedAlias(newEmailAlias)
  }
  React.useEffect(() => {
    if (mode == ViewMode.Create) {
      regenerateAlias()
    }
    mappingService.getAccountEmail().then(email => setMainEmail(email ?? ''))
    if (bubble) {
      mappingService.getAliases().then(aliases => {
        setLimitReached(aliases.length >= MAX_ALIASES)
      })
    }
  }, [])
  return (
    <S.InnerModal>
      <h2>{mode == ViewMode.Create ? getLocale('emailAliasesCreateAliasTitle') : getLocale('emailAliasesEditAliasTitle')}</h2>
      {bubble && <div>{getLocale('emailAliasesBubbleDescription')}</div>}
      {(bubble && limitReached) ?
        <h3 style={{margin: '1em'}}>{getLocale('emailAliasesBubbleLimitReached')}</h3> :
        <span>
          <S.ModalSectionCol style={{}}>
            <h3 style={{ margin: '0.25em' }}>{getLocale('emailAliasesAliasLabel')}</h3>
            <S.GeneratedEmailContainer>
              <div>{proposedAlias}</div>
              {mode == ViewMode.Create && <RefreshButton onClicked={regenerateAlias} />}
            </S.GeneratedEmailContainer>
            <div>{formatMessage(getLocale('emailAliasesEmailsWillBeForwardedTo'), { placeholders: { $1: mainEmail } })}</div>
          </S.ModalSectionCol>
          <S.ModalSectionCol>
            <h3 style={{ margin: '0.25em' }}>{getLocale('emailAliasesNoteLabel')}</h3>
            <Input id='note-input'
              type='text'
              placeholder={getLocale('emailAliasesEditNotePlaceholder')}
              maxlength={255}
              value={proposedNote}
              onChange={(detail: InputEventDetail) => setProposedNote(detail.value)}
              onKeyDown={onEnterKey(createOrSave)}
              style='margin: 0.25em 0em'>
            </Input>
            {mode == ViewMode.Edit && viewState?.alias?.domains && <div>getLocale('emailAliasesUsedBy', viewState?.alias?.domains?.join(', '))</div>}
          </S.ModalSectionCol>
        </span>
      }
      <S.ButtonRow style={{ justifyContent: bubble ? 'space-between' : 'end' }}>
        <span>
          {bubble && <Button onClick={() => mappingService.showSettingsPage()} kind='plain' style='flex-grow: 0;'>
            {getLocale('emailAliasesManageButton')}
          </Button>}
        </span>
        <span>
          <Button onClick={() => returnToMain()} kind='plain' style='flex-grow: 0;'>
            {getLocale('emailAliasesCancelButton')}
          </Button>
          <Button
            style='flex-grow: 0; margin-inline-start: 1em;'
            kind='filled'
            isDisabled={limitReached}
            onClick={() => createOrSave()}>
            {mode == ViewMode.Create ? getLocale('emailAliasesCreateAliasButton') : getLocale('emailAliasesSaveAliasButton')}
          </Button>
        </span>
      </S.ButtonRow>
    </S.InnerModal>
  )
}

const BeforeSendingEmailForm = ({ initEmail, onSubmit }: { initEmail: string, onSubmit: Function }) => {
  const [email, setEmail] = React.useState<string>(initEmail)
  return (<S.Col>
    <h3>{getLocale('emailAliasesSignInOrCreateAccount')}</h3>
    <div style={{ marginBottom: '1em' }}>{getLocale('emailAliasesEnterEmailToGetLoginLink')}</div>
      <S.Row>
        <Input autofocus={true}
          onChange={(detail: InputEventDetail) => setEmail(detail.value)}
          onKeyDown={onEnterKey(() => onSubmit(email))}
          name='email'
          style='flex-grow: 4; margin-inline-end: 1em;'
          type='text'
          placeholder={getLocale('emailAliasesEmailAddressPlaceholder')}
          value={email || ''}
        ></Input>
        <Button onClick={() => onSubmit(email)} type='submit' style='flex-grow: 1' kind='filled'>{getLocale('emailAliasesGetLoginLinkButton')}</Button>
      </S.Row>
  </S.Col>
  )
}

const AfterSendingEmailMessage = ({mainEmail, tryAgain}: {mainEmail: string, tryAgain: Function}) => (
  <S.Col style={{flexGrow: 1}}>
    <h3>{formatMessage(getLocale('emailAliasesLoginEmailOnTheWay'), { placeholders: { $1: mainEmail } })}</h3>
    <div style={{ marginBottom: '1em' }}>{getLocale('emailAliasesClickOnSecureLogin')}</div>
    <div style={{ marginBottom: '1em' }}>{getLocale('emailAliasesDontSeeEmail')} <a href='#' onClick={(e) => { e.preventDefault(); tryAgain()}}>{getLocale('emailAliasesTryAgain')}</a>
    </div>
  </S.Col>
)

const MainEmailEntryForm = ({viewState, mainEmail, onEmailSubmitted, restart} : {viewState:ViewState, mainEmail: string, onEmailSubmitted: Function, restart: Function}) => (
  <S.Card id='main-email-entry-form'>
    <S.SignupRow>
      <BraveIcon style={{marginTop: '1em'}}/>
      {viewState.mode === ViewMode.SignUp ? (<BeforeSendingEmailForm initEmail={mainEmail} onSubmit={onEmailSubmitted}/>) : (<AfterSendingEmailMessage mainEmail={mainEmail} tryAgain={restart}/>)}
    </S.SignupRow>
  </S.Card>
)

const MainView = ({
  viewState, mainEmail, onLogout, aliasesState, setViewState,
  mappingService, onListChange
}: {
  viewState: ViewState,
  mainEmail: string,
  onLogout: Function,
  aliasesState: Alias[],
  setViewState: Function,
  mappingService: MappingService,
  onListChange: Function
}) => (
  (viewState.mode === ViewMode.Startup ?
    (<S.Row style={{margin: '1em', flexGrow: 1, justifyContent: 'center', alignItems: 'center'}}><Icon name='loading-spinner' />
      <h3 style={{margin: '0.25em'}}>{getLocale('emailAliasesConnectingToBraveAccount')}</h3>
     </S.Row>) :
    (<span>
      <MainEmailDisplay onLogout={onLogout} email={mainEmail} />
      <AliasList aliases={aliasesState} onViewChange={setViewState}
        mappingService={mappingService}
        onListChange={onListChange}></AliasList>
    </span>))
)

export const ManagePage = ({ mappingService }:
  {
    mappingService: MappingService
  }) => {
  const [viewState, setViewState] = React.useState<ViewState>({ mode: ViewMode.Startup })
  const [mainEmail, setMainEmail] = React.useState<string>('')
  const mode = viewState.mode
  const [aliasesState, setAliasesState] = React.useState<Alias[]>([]);
  const onEmailChange = async () => {
    const email = await mappingService.getAccountEmail()
    setMainEmail(email ?? '')
    setViewState({ mode: email ? ViewMode.Main : ViewMode.SignUp })
  }
  const onListChange = async () => {
    const aliases = await mappingService.getAliases()
    setAliasesState(aliases)
  }
  const onMainEmailSubmitted = async (email: string) => {
    setMainEmail(email)
    await mappingService.requestAccount(email)
    setViewState({ mode: ViewMode.AwaitingAuthorization })
    const accountReady = await mappingService.onAccountReady()
    if (accountReady) {
      setViewState({ mode: ViewMode.Main })
      await onListChange()
    }
  }
  const onLogout = () => {
    mappingService.logout()
    setViewState({ mode: ViewMode.SignUp })
  }
  const restart = async () => {
    await mappingService.cancelAccountRequest()
    setViewState({ mode: ViewMode.SignUp })
  }
  const returnToMain = () => {
    setViewState({ mode: ViewMode.Main })
    onListChange()
  }
  React.useEffect(() => {
    onEmailChange();
    onListChange();
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'visible') {
        onEmailChange();
        onListChange();
      }
    })
  }, [] /* Only run at mount. */)
  return (
    <S.Col style={{ padding: spacing.l }}>
      <Introduction />
      {viewState.mode === ViewMode.SignUp || viewState.mode === ViewMode.AwaitingAuthorization ?
        (<MainEmailEntryForm viewState={viewState} mainEmail={mainEmail} onEmailSubmitted={onMainEmailSubmitted} restart={restart}/>) :
        <MainView viewState={viewState} mainEmail={mainEmail} onLogout={onLogout} aliasesState={aliasesState} setViewState={setViewState} mappingService={mappingService} onListChange={onListChange}/>}
      {(mode == ViewMode.Create || mode == ViewMode.Edit) &&
        (<span><S.GrayOverlay onClick={returnToMain}>&nbsp;</S.GrayOverlay>
          <ModalWithCloseButton returnToMain={returnToMain}>
            <EmailAliasModal
              returnToMain={returnToMain}
              viewState={viewState}
              email={mainEmail}
              mode={mode}
              mappingService={mappingService} />
          </ModalWithCloseButton>
        </span>)}
    </S.Col>
  )
}

export const mount = (at: HTMLElement, mappingService: MappingService) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <ManagePage mappingService={mappingService}/>
    </StyleSheetManager>
  )
}

export const mountModal = (at: HTMLElement, mappingService: MappingService) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <EmailAliasModal
       returnToMain={() => mappingService.closeBubble()}
       bubble={true}
       mode={ViewMode.Create}
       email={'test@test.com'}
       mappingService={mappingService}/>
    </StyleSheetManager>
  )
}

  ; (window as any).mountEmailAliases = mount
  ; (window as any).mountModal = mountModal
