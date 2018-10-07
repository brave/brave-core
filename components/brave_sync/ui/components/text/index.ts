/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import Heading from '../../../components/text/heading'

export const Title = styled(Heading)`
  font-weight: 500;
  font-size: 26px;
  margin: 0 0 20px 0;
`

export const SubTitle = styled(Heading)`
  font-weight: 500;
  font-size: 20px;
  margin: 30px 0 10px 0;
`

export const Paragraph = styled<{}, 'p'>('p')`
  font-size: 16px;
  font-weight: 300;
  line-height: 1.75;
  margin: 0;
  font-family: 'Muli', sans-serif;
  padding: 0 0 10px 0
`

export const EmphasisText = styled(Paragraph.withComponent('em'))`
  display: block;
  font-weight: 400;
  margin: 0px 0 10px;
  font-style: normal;
`

export const SecondaryText = styled(EmphasisText)`
  color: #7C7D8C;
  margin: 0px 0 20px;
`

export const Link = styled<{}, 'a'>('a')`
  color: ${p => p.theme.color.brandBrave};
  font-weight: bold;
  margin: 0 10px;
  text-decoration: none;
  display: inline;
`

export const Label = styled<{}, 'label'>('label')`
  font-family: ${p => p.theme.fontFamily.body};
  color: ${p => p.theme.color.defaultControlActive};
  margin: 0;
  font-size: 14px;
  font-weight: 400;
  letter-spacing: normal;
`

export const SwitchLabel = styled(Label)`
  margin: 0 10px;
`

export const List = styled<{}, 'ul'>('ul')`
  font-size: 16px;
  font-weight: 300;
  margin: 20px 0;
  padding: 0 0 0 20px;
  list-style-type: decimal;
`

export const ListOrdered = List.withComponent('ol')

export const ListBullet = styled(Paragraph.withComponent('li'))`

  .syncButton {
    margin: 15px 0
  }
`
