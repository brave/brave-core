import * as React from 'react'
import styled from 'styled-components'
import Flex from '../Flex'
import Discover from './Discover'
import { BackArrow, Cross } from './Icons'
import Button from '$web-components/button'
import Toggle from '$web-components/toggle'
import SourcesList from './SourcesList'
import DisabledPlaceholder from './DisabledPlaceholder'
import { useNewTabPref } from '../../hooks/usePref'
import { useBraveNews } from './Context'
import { getLocale } from '../../../common/locale'
import { formatMessage } from '../../../brave_rewards/resources/shared/lib/locale_context'

const Grid = styled.div`
  width: 100%;
  height: 100%;

  display: grid;
  grid-template-columns: 250px auto;
  grid-template-rows: 64px 2px auto;

  grid-template-areas:
    "back-button header"
    "separator separator"
    "sidebar content";
`

const Header = styled(Flex)`
  grid-area: header;
  padding: 24px;
`

const HeaderText = styled.span`
  font-size: 16px;
  font-weight: 500;
`

const CloseButton = styled(Button)`
  --inner-border-size: 0;
  --outer-border-size: 0;
  padding: 12px;
  width: 32px;
  height: 32px;
`

const BackButtonContainer = styled.div`
  grid-area: back-button;
  align-items: center;
  display: flex;
  padding: 12px;
`

const BackButton = styled(Button)`
  --inner-border-size: 0;
  --outer-border-size: 0;
`

const BackButtonText = styled.span`
  font-size: 12px;
  line-height: 1;
`

const Hr = styled.hr`
  grid-area: separator;
  width: 100%;
  align-self: center;
  background: var(--divider1);
  height: 2px;
  border-width: 0;
`

const Sidebar = styled.div`
  position: relative;
  overflow: auto;
  grid-area: sidebar;
  padding: 28px 32px;
  background: var(--background2);
`

// Overlay on top of the sidebar, shown when it is disabled.
const SidebarOverlay = styled.div`
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  background: var(--background1);
  opacity: 0.7;
`

const Content = styled.div`
  grid-area: content;
  overflow: auto;
  padding: 20px 64px;
`

export default function Configure () {
  const [enabled, setEnabled] = useNewTabPref('isBraveTodayOptedIn')
  const { setCustomizePage: setPage } = useBraveNews()

  let content: JSX.Element
  if (!enabled) {
    content = <DisabledPlaceholder enableBraveNews={() => setEnabled(true)} />
  } else {
    content = <Discover />
  }

  return (
    <Grid>
      <BackButtonContainer>
        <BackButton onClick={() => setPage(null)}>
          {BackArrow}
          <BackButtonText>
            {formatMessage(getLocale('braveNewsBackToDashboard'), {
              tags: {
                $1: content => <strong key="$1">{content}</strong>
              }
            })}
          </BackButtonText>
        </BackButton>
      </BackButtonContainer>
      <Header direction="row-reverse" gap={12} align="center" justify="space-between">
        <CloseButton onClick={() => setPage(null)}>{Cross}</CloseButton>
        {enabled && <Flex direction="row" align="center" gap={8}>
          <HeaderText>{getLocale('braveTodayTitle')}</HeaderText>
          <Toggle isOn={enabled} onChange={setEnabled} />
        </Flex>}
      </Header>
      <Hr />
      <Sidebar>
        <SourcesList />
        {!enabled && <SidebarOverlay />}
      </Sidebar>
      <Content>
        {content}
      </Content>
    </Grid>
  )
}
