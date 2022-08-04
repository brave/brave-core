import * as React from 'react'

import * as S from './style'
import fontSerifSvg from '../../svg/fontSerif'
import fontSansSvg from '../../svg/fontSans'
import fontMonoSvg from '../../svg/fontMono'
import fontDyslexicSvg from '../../svg/fontDyslexic'
import contentTextOnlySvg from '../../svg/contentTextOnly'
import contentTextWithImagesSvg from '../../svg/contentTextWithImages'

const fontStyleOptions = [
  {
    title: 'Sans',
    svgIcon: fontSansSvg
  },
  {
    title: 'Serif',
    svgIcon: fontSerifSvg
  },
  {
    title: 'Mono',
    svgIcon: fontMonoSvg
  },
  {
    title: 'Dyslexic',
    svgIcon: fontDyslexicSvg
  }
]

const contentStyleOptions = [
  {
    title: 'Text with images',
    svgIcon: contentTextWithImagesSvg
  },
  {
    title: 'Text only',
    svgIcon: contentTextOnlySvg
  }
]

type OptionType = {
  isSelected: boolean
  children: JSX.Element
  onClick?: Function
  ariaLabel?: string
}

function ListBox (props: React.PropsWithChildren<{}>) {
  return (
    <S.Box role="listbox" aria-orientation="horizontal">
      {props.children}
    </S.Box>
  )
}

function Option (props: OptionType) {
  const handleClick = () => {
    props.onClick?.()
  }

  return (
    <button
      role="option"
      className={props.isSelected ? 'is-active' : ''}
      aria-selected={props.isSelected}
      aria-label={props?.ariaLabel}
      onClick={handleClick}
    >
      {props.children}
    </button>
  )
}

export function FontStyleList () {
  const [activeOption] = React.useState('sans')

  return (
    <ListBox>
      {fontStyleOptions.map(entry => {
        return (
          <Option
            key={entry.title}
            isSelected={activeOption === entry.title.toLocaleLowerCase()}
          >
            <div className="sm">
              <div>{<entry.svgIcon />}</div>
              {entry.title}
            </div>
          </Option>
        )
      })}
    </ListBox>
  )
}

export function ContentList () {
  const [activeOption] = React.useState('text with images')

  return (
    <ListBox>
      {contentStyleOptions.map(entry => {
        return (
          <Option
            key={entry.title}
            isSelected={activeOption === entry.title.toLocaleLowerCase()}
            ariaLabel={entry.title}
          >
            <div>{<entry.svgIcon />}</div>
          </Option>
        )
      })}
    </ListBox>
  )
}
