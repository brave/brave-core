import * as React from 'react'

import * as S from './style'
import Toggle from '$web-components/toggle'
import { FontStyleList, ContentList } from '../lists'
import ZoomControl from '../zoom-control'
import ThemeControl from '../theme-control'

function MainPanel () {
  return (
    <S.Box>
      <S.HeaderBox>
        <S.HeaderContent>
          <div>
            <S.SiteName>nytimes.com</S.SiteName>
            in Speedreader
          </div>
          <div>
            <Toggle
              brand="shields"
              isOn={true}
            />
          </div>
        </S.HeaderContent>
      </S.HeaderBox>
      <S.Section>
        <div className="title">Theme</div>
        <ThemeControl />
      </S.Section>
      <S.Section>
        <div className="title">Font style</div>
        <FontStyleList />
        <br />
        <ZoomControl />
      </S.Section>
      <S.Section>
        <div className="title">Content</div>
        <ContentList />
      </S.Section>
    </S.Box>
  )
}

export default MainPanel
