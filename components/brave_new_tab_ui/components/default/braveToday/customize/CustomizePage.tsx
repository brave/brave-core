import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '../../../../../common/locale'
import Button from '../../../../../web-components/button'
import Flex from '../../../Flex'
import { useBraveNews } from './Context'
import { BackArrow } from './Icons'

const BackButtonContainer = styled.div`
  all: unset;
  flex: 1;

  &> button {
    --inner-border-size: 0;
    --outer-border-size: 0;
    padding: 0;

    &:hover {
      --inner-border-size: 0;
      --outer-border-size: 0;
    }
  }
`

const Header = styled.span`
  font-weight: 500;
  font-size: 16px;
  color: var(--text01);
  flex: 5;
  text-align: center;
`

const Spacer = styled.div`flex: 1;`

export default function CustomizePage (props: {
  title: string
  children: React.ReactNode
}) {
  const { setCustomizePage } = useBraveNews()
  return <Flex direction="column">
    <Flex align="center">
      <BackButtonContainer>
        <Button onClick={() => setCustomizePage('news')}>
          {BackArrow}
          {getLocale('braveNewsBackButton')}
        </Button>
      </BackButtonContainer>
      <Header>{props.title}</Header>
      <Spacer />
    </Flex>
    {props.children}
  </Flex>
}
