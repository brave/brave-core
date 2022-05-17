import styled from 'styled-components'

interface StyleProps {
  position: 'left' | 'right' | 'center'
  verticalPosition: 'above' | 'below'
  isAddress?: boolean
  horizontalMarginPx?: number
}

export const TipAndChildrenWrapper = styled.div`
  position: relative;
  cursor: default;
`

export const TipWrapper = styled.div<StyleProps>`
  position: absolute;
  left: ${(p) =>
    p.position === 'right'
      ? 'unset'
      : p.position === 'left'
        ? `${p?.horizontalMarginPx || 0}px`
        : '50%'
  };

  right: ${(p) =>
    p.position === 'right'
      ? `${p?.horizontalMarginPx || 0}px`
      : 'unset'
  };

  transform: 
    translateX(${
      (p) => p.position === 'center'
        ? '-50%'
        : '0'
    })
    translateY(${
      (p) => p.verticalPosition === 'below'
        ? '0px'
        : '-105%'
    });

    z-index: 100;
`

export const Tip = styled.div<StyleProps>`
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

  margin-top: ${(p) => p.verticalPosition === 'below'
    ? 'unset'
    : '-1px'
  };
  
  margin-bottom: ${(p) => p.verticalPosition === 'above'
    ? '0px'
    : '-1px'
  };
  
  margin-left: ${(p) =>
    p.position === 'right'
      ? 'unset'
      : p.position === 'left'
        ? '25px'
        : '50%'
  };
  
  margin-right: ${(p) =>
    p.position === 'right'
      ? '25px'
      : 'unset'
  };
  
  transform: 
    translateX(${(p) => p.position === 'center'
      ? '-50%'
      : '0'
    })
    rotate(${(p) => p.verticalPosition === 'below'
      ? '0deg'
      : '180deg'
    });

`

export const ActionNotification = styled(Tip)`
  background: ${p => p.theme.palette.blurple500};
`
