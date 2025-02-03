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
import { loadTimeData } from '$web-common/loadTimeData'
import {
  AccountRow,
  AliasAnnotation,
  AliasControls,
  AliasItemRow,
  AliasListIntro,
  BraveIconCircle,
  BraveIconWrapper,
  ButtonRow,
  Card,
  CloseButton,
  Col,
  CopyButtonWrapper,
  EmailContainer,
  GeneratedEmailContainer,
  GrayOverlay,
  MainEmail,
  MainEmailDescription,
  MainEmailTextContainer,
  ManageAccountLink,
  MenuButton,
  Modal,
  ModalSectionCol,
  Row,
  SignupRow,
} from './styles'

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

const BraveIcon = ({style}: {style?: React.CSSProperties | undefined}) => (
  <BraveIconCircle style={{...style, flexGrow: 0}}>
    <BraveIconWrapper>
      <Icon name='brave-icon-release-color' />
    </BraveIconWrapper>
  </BraveIconCircle>
)

const Introduction = () => (
  <Card id='introduction'>
    <h2>{loadTimeData.getString('emailAliasesShortDescription')}</h2>
    <div>{loadTimeData.getString('emailAliasesDescription')} <a href="https://support.brave.com" target='_blank'>{loadTimeData.getString('emailAliasesLearnMore')}</a>
    </div>
  </Card>
)

const MainEmailDisplay = ({ email, onLogout }: { email: string, onLogout: Function }) => (
  <Card id='main-email-display'>
    <AccountRow>
    <Row>
      <BraveIcon />
      <MainEmailTextContainer>
        <MainEmail>{email === '' ? loadTimeData.getString('emailAliasesConnectingToBraveAccount') : email}</MainEmail>
        <MainEmailDescription>{loadTimeData.getString('emailAliasesBraveAccount')}</MainEmailDescription>
      </MainEmailTextContainer>
    </Row>
    <ManageAccountLink
      title={loadTimeData.getString('emailAliasesSignOutTitle')}
      href='#'
      onClick={(e) => { e.preventDefault(); onLogout() }}>
      <Icon name="outside" />
      <span style={{ margin: '0.25em' }}>{loadTimeData.getString('emailAliasesSignOut')}</span>
    </ManageAccountLink>
  </AccountRow>
</Card>
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
    <Row style={{ fontSize: '90%' }}>
      <Icon name={iconName} />
      <span style={{ marginInlineStart: '0.5em' }}>{text}</span>
    </Row>
  </leo-menu-item>
)

const CopyToast = ({ children }: React.PropsWithChildren) => {
  const [copied, setCopied] = React.useState<boolean>(false)
  return (<div onClick={() => {
    setCopied(true)
    setTimeout(() => setCopied(false), 1000)
  }}>
    <Tooltip text={copied ? loadTimeData.getString('emailAliasesCopiedToClipboard') : ''} mode="mini" visible={copied}>
      {children}
    </Tooltip>
  </div>
  )
}

const AliasItem = ({ alias, onEdit, onDelete }: { alias: Alias, onEdit: Function, onDelete: Function }) => {
  return (
    <AliasItemRow>
      <Col>
        <CopyToast>
          <EmailContainer title={loadTimeData.getString('emailAliasesClickToCopyAlias')}
            onClick={(event: React.MouseEvent<HTMLElement>) => copyEmailToClipboard(alias.email)}>
            {alias.email}
          </EmailContainer>
        </CopyToast>
        {((alias.note || alias.domains) &&
          <AliasAnnotation>
            {(alias.note && <span>{alias.note}</span>)}
            {alias.domains && alias.note && <span>. </span>}
            {(alias.domains && <span>{loadTimeData.getStringF('emailAliasesUsedBy', alias.domains?.join(", "))}</span>)}
          </AliasAnnotation>
        )}
      </Col>
      <AliasControls>
        <CopyToast>
          <CopyButtonWrapper
            title={loadTimeData.getString('emailAliasesClickToCopyAlias')}
            onClick={() => {
              copyEmailToClipboard(alias.email)
            }}>
            <Icon name="copy" style={{color: color.text.secondary}}/>
          </CopyButtonWrapper>
        </CopyToast>
        <ButtonMenu>
          <MenuButton slot='anchor-content' kind='plain-faint' size="large" style='width: 1.5em;'>
            <Icon name="more-vertical" />
          </MenuButton>
          <AliasMenuItem
            iconName="edit-pencil"
            text={loadTimeData.getString('emailAliasesEdit')}
            onClick={() => onEdit()} />
          <AliasMenuItem
            iconName="trash"
            text={loadTimeData.getString('emailAliasesDelete')}
            onClick={() => onDelete(alias)} />
        </ButtonMenu>
      </AliasControls>
    </AliasItemRow>
  )
}

const AliasList = ({ aliases, onViewChange, onListChange, mappingService }: { mappingService: MappingService, aliases: Alias[], onViewChange: Function, onListChange: Function }) => (
  <Card style={{ borderTop: `1px solid ${color.legacy.divider1}` }}>
    <AliasListIntro>
      <Col>
        <h2>{loadTimeData.getString('emailAliasesListTitle')}</h2>
        <div>
          {loadTimeData.getString('emailAliasesCreateDescription')}
        </div>
      </Col>
      <Button style='flex-grow: 0;'
        title={loadTimeData.getString('emailAliasesCreateAliasTitle')}
        id='add-alias'
        onClick={
          async () => {
            onViewChange({ mode: ViewMode.Create })
            const newEmailAlias = await mappingService.generateAlias()
            onViewChange({ mode: ViewMode.Create, alias: { email: newEmailAlias } })
          }
        }>
        {loadTimeData.getString('emailAliasesCreateAliasLabel')}
      </Button>
    </AliasListIntro>
    {aliases.map(
      alias => <AliasItem
        key={alias.email}
        alias={alias}
        onEdit={() => onViewChange({ mode: ViewMode.Edit, alias: alias })}
        onDelete={async (alias: Alias) => {
          await mappingService.deleteAlias(alias.email)
          onListChange()
        }}></AliasItem>)}
  </Card>
)


const RefreshButton = ( { onClicked } : { onClicked: Function }) => (
  <Button title={loadTimeData.getString('emailAliasesRefreshButtonTitle')}
    onClick={() => onClicked()}
    kind="plain" style='flex-grow: 0; padding: 0px'>
    <Icon name="refresh" />
  </Button>
)

const ModalWithCloseButton = ({ children, returnToMain }: React.PropsWithChildren & { returnToMain: Function }) => (
  <Modal>
    <CloseButton onClick={() => returnToMain()}><Icon name='close' /></CloseButton>
    {children}
  </Modal>
)

export const EmailAliasModal = (
  { returnToMain, viewState, email, mode, mappingService }:
    { returnToMain: Function,
      viewState?: ViewState,
      mode: ViewMode
      email: string,
      mappingService: MappingService }
) => {
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
  }, [])
  return (
    <span>
      <h2>{mode == ViewMode.Create ? loadTimeData.getString('emailAliasesCreateAliasTitle') : loadTimeData.getString('emailAliasesEditAliasTitle')}</h2>
      <ModalSectionCol style={{}}>
        <h3 style={{ margin: '0.25em' }}>{loadTimeData.getString('emailAliasesAliasLabel')}</h3>
      <GeneratedEmailContainer>
        <div>{proposedAlias}</div>
        {mode == ViewMode.Create && <RefreshButton onClicked={regenerateAlias} />}
      </GeneratedEmailContainer>
      <div>{loadTimeData.getStringF('emailAliasesEmailsWillBeForwardedTo', mainEmail)}</div>
    </ModalSectionCol>
    <ModalSectionCol>
      <h3 style={{ margin: '0.25em' }}>{loadTimeData.getString('emailAliasesNoteLabel')}</h3>
      <Input id='note-input'
        type='text'
        placeholder={loadTimeData.getString('emailAliasesEditNotePlaceholder')}
        value={proposedNote}
        onChange={(detail: InputEventDetail) => setProposedNote(detail.value)}
        onKeyDown={onEnterKey(createOrSave)}
        style='margin: 0.25em 0em'>
      </Input>
      {mode == ViewMode.Edit && viewState?.alias?.domains && <div>loadTimeData.getStringF('emailAliasesUsedBy', viewState?.alias?.domains?.join(', '))</div>}
    </ModalSectionCol>
    <ButtonRow>
      <Button onClick={() => returnToMain()} kind='plain' style='flex-grow: 0;'>
        {loadTimeData.getString('emailAliasesCancelButton')}
      </Button>
      <Button
        style='flex-grow: 0; margin-inline-start: 1em;'
        kind='filled'
        onClick={() => createOrSave()}>
        {mode == ViewMode.Create ? loadTimeData.getString('emailAliasesCreateAliasButton') : loadTimeData.getString('emailAliasesSaveAliasButton')}
        </Button>
      </ButtonRow>
    </span>
  )
}

const BeforeSendingEmailForm = ({ initEmail, onSubmit }: { initEmail: string, onSubmit: Function }) => {
  const [email, setEmail] = React.useState<string>(initEmail)
  return (<Col>
    <h3>{loadTimeData.getString('emailAliasesSignInOrCreateAccount')}</h3>
    <div style={{ marginBottom: '1em' }}>{loadTimeData.getString('emailAliasesEnterEmailToGetLoginLink')}</div>
      <Row>
        <Input autofocus={true}
          onChange={(detail: InputEventDetail) => setEmail(detail.value)}
          onKeyDown={onEnterKey(() => onSubmit(email))}
          name='email'
          style='flex-grow: 4; margin-inline-end: 1em;'
          type='text'
          placeholder={loadTimeData.getString('emailAliasesEmailAddressPlaceholder')}
          value={email || ''}
        ></Input>
        <Button onClick={() => onSubmit(email)} type='submit' style='flex-grow: 1' kind='filled'>{loadTimeData.getString('emailAliasesGetLoginLinkButton')}</Button>
      </Row>
  </Col>
  )
}

const AfterSendingEmailMessage = ({mainEmail, tryAgain}: {mainEmail: string, tryAgain: Function}) => (
  <Col style={{flexGrow: 1}}>
    <h3>{loadTimeData.getStringF('emailAliasesLoginEmailOnTheWay', mainEmail)}</h3>
    <div style={{ marginBottom: '1em' }}>{loadTimeData.getString('emailAliasesClickOnSecureLogin')}</div>
    <div style={{ marginBottom: '1em' }}>{loadTimeData.getString('emailAliasesDontSeeEmail')} <a href='#' onClick={(e) => { e.preventDefault(); tryAgain()}}>{loadTimeData.getString('emailAliasesTryAgain')}</a>
    </div>
  </Col>
)

const MainEmailEntryForm = ({viewState, mainEmail, onEmailSubmitted, restart} : {viewState:ViewState, mainEmail: string, onEmailSubmitted: Function, restart: Function}) => (
  <Card id='main-email-entry-form'>
    <SignupRow>
      <BraveIcon style={{marginTop: '1em'}}/>
      {viewState.mode === ViewMode.SignUp ? (<BeforeSendingEmailForm initEmail={mainEmail} onSubmit={onEmailSubmitted}/>) : (<AfterSendingEmailMessage mainEmail={mainEmail} tryAgain={restart}/>)}
    </SignupRow>
  </Card>
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
  }, [] /* Only run at mount. */)
  return (
    <Col style={{ padding: spacing.l }}>
      <Introduction />
      {viewState.mode === ViewMode.SignUp || viewState.mode === ViewMode.AwaitingAuthorization ?
        (<MainEmailEntryForm viewState={viewState} mainEmail={mainEmail} onEmailSubmitted={onMainEmailSubmitted} restart={restart}/>) :
        (viewState.mode === ViewMode.Startup ?
          (<Row style={{margin: '1em', flexGrow: 1, justifyContent: 'center', alignItems: 'center'}}><Icon name='loading-spinner' />
            <h3 style={{margin: '0.25em'}}>{loadTimeData.getString('emailAliasesConnectingToBraveAccount')}</h3>
           </Row>) :
          (<span>
            <MainEmailDisplay onLogout={onLogout} email={mainEmail} />
            <AliasList aliases={aliasesState} onViewChange={setViewState}
              mappingService={mappingService}
              onListChange={onListChange}></AliasList>
          </span>))}
      {(mode == ViewMode.Create || mode == ViewMode.Edit) &&
        (<span><GrayOverlay onClick={returnToMain}>&nbsp;</GrayOverlay>
          <ModalWithCloseButton returnToMain={returnToMain}>
            <EmailAliasModal
              returnToMain={returnToMain}
              viewState={viewState}
              email={mainEmail}
              mode={mode}
              mappingService={mappingService} />
          </ModalWithCloseButton>
        </span>)}
    </Col>
  )
}

export const mount = (at: HTMLElement, mappingService: MappingService) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <ManagePage {...{mappingService}}/>
    </StyleSheetManager>
  )
}

export const mountModal = (at: HTMLElement, mappingService: MappingService) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <EmailAliasModal
       returnToMain={() => mappingService.closeBubble()}
       mode={ViewMode.Create}
       email={'test@test.com'}
       mappingService={mappingService}/>
    </StyleSheetManager>
  )
}

  ; (window as any).mountEmailAliases = mount
  ; (window as any).mountModal = mountModal
