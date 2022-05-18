import styled, { css } from 'styled-components'

interface StyleProps {
  position: 'left' | 'right' | 'center'
  verticalPosition: 'above' | 'below'
  isAddress?: boolean
  horizontalMargin?: string
}

export const TipAndChildrenWrapper = styled.div`
  position: relative;
  cursor: default;
`

export const TipWrapper = styled.div<StyleProps>`
  position: absolute;
  
  transform: 
    translateX(${
      (p) => p.position === 'center'
        ? '0%'
        : p?.horizontalMargin
          ? p.position === 'left'
            ? '-' + p.horizontalMargin
            : '' + p.horizontalMargin
        : '0'
    })
    translateY(${
      (p) => p.verticalPosition === 'below'
        ? '0px'
        : '-105%'
    });

    z-index: 100;
`

export const Tip = styled.div<{ isAddress?: boolean }>`
  border-radius: 4px;
  padding: 6px;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;

  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};

  white-space: ${(p) => p.isAddress ? 'pre-line' : 'nowrap'};
  width: ${(p) => p.isAddress ? '180px' : 'unset'};
  word-break: ${(p) => p.isAddress ? 'break-all' : 'unset'};
`

export const Pointer = styled.div<StyleProps>`
  width: 0;
  height: 0;
  border-style: solid;
  border-width: 0 7px 8px 7px;

  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
  
  ${(p) => p.position === 'center' && css`
    margin: 0 auto;
  `}

  transform: 
    translateX(${(p) => p.position === 'center'
      ? '0'
      : p.position === 'right'
        ? '-25px'
        : '25px'
    })
    translateY(${(p) => p.verticalPosition === 'above'
      ? '-1px'
      : '1px'
    })
    rotate(${(p) => p.verticalPosition === 'below'
      ? '0deg'
      : '180deg'
    });

`

export const ActionNotification = styled(Tip)`
  background: ${p => p.theme.palette.blurple500};
`
