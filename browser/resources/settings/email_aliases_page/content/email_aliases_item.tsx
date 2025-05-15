import ButtonMenu from '@brave/leo/react/buttonMenu'
import { formatLocale } from '$web-common/locale'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import { Alias } from "gen/brave/components/email_aliases/email_aliases.mojom.m"
import * as React from 'react'
import Button from '@brave/leo/react/button'
import Col from './styles/Col'
import Row from './styles/Row'
import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import Tooltip from '@brave/leo/react/tooltip'
import { onEnterKeyForDiv } from './on_enter_key'

const AliasItemRow = styled(Row)`
  font: ${font.default.regular};
  padding: ${spacing.l} ${spacing['2Xl']};
  border-top: ${color.divider.subtle} 1px solid;
  justify-content: space-between;
`

const AliasAnnotation = styled.div`
  font: ${font.small.regular};
  color: ${color.text.secondary};
`

const EmailContainer = styled.div`
  cursor: pointer;
  font: ${font.default.regular};
`

const AliasControls = styled(Row)`
  user-select: none;
  column-gap: ${spacing.m};
  * {
    --leo-icon-color: ${color.icon.default};
  }
`

const AliasMenuItemContent = styled(Row)`
  gap: ${spacing.m};
  padding: 0;
`

const AliasMenuItem = ({ onClick, iconName, text }:
  { onClick: EventListener, iconName: string, text: string }) =>
  <leo-menu-item
    onClick={onClick}>
    <AliasMenuItemContent>
      <Icon name={iconName} />
      <span>{text}</span>
    </AliasMenuItemContent>
  </leo-menu-item>

const CopyToast = ({ text, tabIndex, children }:
  { text: string, tabIndex?: number, children: React.ReactNode }) => {
  const [copied, setCopied] = React.useState<boolean>(false)
  const copy = () => {
    navigator.clipboard.writeText(text)
    setCopied(true)
    setTimeout(() => setCopied(false), 1000)
  }
  return <div
    tabIndex={tabIndex}
    data-testid="copy-toast"
    onKeyDown={onEnterKeyForDiv(copy)}
    onClick={copy}>
    <Tooltip text={copied ? getLocale('emailAliasesCopiedToClipboard') : ''}
             mode="mini" visible={copied}>
      {children}
    </Tooltip>
  </div>
}

export const AliasItem = ({ alias, onEdit, onDelete }:
  { alias: Alias, onEdit: () => void, onDelete: () => void }) =>
  <AliasItemRow>
    <Col>
      <CopyToast text={alias.email}>
        <EmailContainer title={getLocale('emailAliasesClickToCopyAlias')}>
            {alias.email}
          </EmailContainer>
        </CopyToast>
        {(alias.note || alias.domains) &&
          <AliasAnnotation>
            {alias.note}
            {alias.domains && alias.note && <span>. </span>}
            {alias.domains && formatLocale('emailAliasesUsedBy',
              { $1: alias.domains?.join(", ") })}
          </AliasAnnotation>}
        </Col>
      <AliasControls>
        <CopyToast text={alias.email} tabIndex={0}>
          <Button
            fab
            title={getLocale('emailAliasesClickToCopyAlias')}
            kind='plain-faint'
            size='medium'>
            <Icon name="copy"/>
          </Button>
        </CopyToast>
        <ButtonMenu anchor-content="more-vertical">
          <Button
            fab
            slot='anchor-content'
            name='more'
            kind='plain-faint'
            size="large">
            <Icon name="more-vertical" />
          </Button>
          <AliasMenuItem
            iconName="edit-pencil"
            text={getLocale('emailAliasesEdit')}
            onClick={onEdit} />
          <AliasMenuItem
            iconName="trash"
            text={getLocale('emailAliasesDelete')}
            onClick={onDelete} />
        </ButtonMenu>
      </AliasControls>
    </AliasItemRow>
