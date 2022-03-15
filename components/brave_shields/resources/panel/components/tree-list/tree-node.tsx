import * as React from 'react'
import styled from 'styled-components'
import UrlElement from './url-element'

const Tree = styled.div`
  display: grid;
  grid-template-columns: 20px 2fr;
  grid-gap: 5px;
  align-items: flex-start;
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 600;
  font-weight: normal;
  line-height: 1.2;
  margin-bottom: 12px;

  &:last-child {
    margin-bottom: 0;
  }
`

const TreeControlBox = styled.span`
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
`

const SVGBox = styled.i`
  position: absolute;
  top: 20px;
  left: 10px;

  path {
    stroke: ${(p) => p.theme.color.text03};
  }
`

const ExpandToggleButton = styled.button`
  --border: 2px solid transparent;
  background-color: transparent;
  padding: 0;
  margin: 0;
  border: var(--border);
  width: 16px;
  height: 14px;
  display: flex;
  align-items: center;
  cursor: pointer;

  &:focus-visible {
    --border: 2px solid ${(p) => p.theme.color.focusBorder};
  }
`

const TreeContents = styled.div`
  overflow: hidden; /* to wrap contents */
`

interface TreeNodeProps {
  host: string
  resourceList: string[]
}

function rectToQuad (rect: DOMRect) {
  return DOMQuad.fromRect({
    x: ('x' in rect) ? rect.x : rect.left,
    y: ('y' in rect) ? rect.y : rect.top,
    width: rect.width,
    height: rect.height
  })
}

function getRelativeBoundingRect (from: DOMRect, to: Element) {
  const fromQuad = rectToQuad(from)
  const toQuad = rectToQuad(to.getBoundingClientRect())

  // We only need x,y coordinates. Adding width/height yields the top position to negative values after element expands. Possibly a bug to fix in the future
  return new DOMRectReadOnly(
    toQuad.p1.x - fromQuad.p1.x,
    toQuad.p2.y - fromQuad.p2.y
  )
}

function TreeNode (props: TreeNodeProps) {
  const treeChildrenBoxRef = React.useRef() as React.MutableRefObject<HTMLDivElement>
  const svgBoxRef = React.useRef() as React.MutableRefObject<HTMLElement>
  const [axisLeftHeight, setAxisLeftHeight] = React.useState(0)
  const [tickValues, setTickValues] = React.useState<number[]>([])
  const [isExpanded, setIsExpanded] = React.useState(false)
  const hasResources = props.resourceList.length > 0

  const measure = () => {
    if (treeChildrenBoxRef.current && svgBoxRef.current) {
      requestAnimationFrame(() => {
        const els = Array.from(treeChildrenBoxRef.current.children)

        // We memoize the rect for SVGBox element as this will never change position
        const svgBoxRect = svgBoxRef.current.getBoundingClientRect()

        // Calculate the vertical center of each element's box. The positions we get from this will be the translate values for our ticks
        const finalValues = els.map((el: HTMLElement) => {
          const rect = getRelativeBoundingRect(svgBoxRect, el)
           // When an element expands, the box height increases and elYCenter gets recalculated. We should always align the tick position to the center of the first line.
          const elYCenter = (parseInt(getComputedStyle(el).getPropertyValue('line-height')) / 2)

          return Math.round(rect.top + elYCenter)
        })

        // TODO(nullhook): Batch state updates
        // We dont let the height of the axis extend to avoid hanging dash
        const height = Math.min(
          finalValues[finalValues.length - 1] + 1, /* pad the value so the position isn't exactly on the edge */
          Math.round(treeChildrenBoxRef.current.offsetHeight)
        )

        setAxisLeftHeight(height)
        setTickValues(finalValues)
      })
    }
  }

  const renderTreeExpandToggle = () => {
    return (
      <ExpandToggleButton
        aria-label="Expand to see sub resources"
        role="button"
        onClick={() => setIsExpanded(x => !x)}
        aria-expanded={isExpanded}
      >
        {isExpanded ? (
          <svg width="12" height="3" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M1.5 1h9" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/></svg>
        ) : (
          <svg width="12" height="12" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M6 11.25a.75.75 0 0 0 .75-.75V6.75h3.75a.75.75 0 0 0 0-1.5H6.75V1.5a.75.75 0 0 0-1.5 0v3.75H1.5a.75.75 0 0 0 0 1.5h3.75v3.75c0 .414.336.75.75.75Z"/></svg>
        )}
      </ExpandToggleButton>
    )
  }

  const renderBullet = () => {
    return (
      <i>
        <svg width="5" height="5" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
          <circle cx="1.5" cy="1.5" r="1.5"/>
        </svg>
      </i>
    )
  }

  const renderLeftAxis = () => {
    return (
      <SVGBox ref={svgBoxRef}>
        <svg width="20" height={axisLeftHeight} fill="currentColor" xmlns="http://www.w3.org/2000/svg">
          <g id="vertical-axis">
            <path
              d={`M.5 0v${axisLeftHeight}`}
              stroke="currentColor"
              strokeLinecap="round"
              strokeLinejoin="round"
              strokeDasharray="2 4"
            />
          </g>
          {
            tickValues.map((value, idx) => {
              return (
                <g key={idx} transform={`translate(0,${value})`}>
                  <path d="M1 .5h12" stroke="currentColor" strokeLinejoin="round" strokeDasharray="2 2" />
                </g>
              )
            })
          }
        </svg>
      </SVGBox>
    )
  }

  // We're using layout effect here so the initial frame doesn't render an empty space for the axis i.e to avoid flash of empty content
  React.useLayoutEffect(() => {
    measure()
  }, [props.resourceList, isExpanded])

  return (
    <Tree>
      <TreeControlBox>
        {hasResources ? renderTreeExpandToggle() : renderBullet()}
        {hasResources && isExpanded ? renderLeftAxis() : null}
      </TreeControlBox>
      <TreeContents>
        <UrlElement name={props.host} isHost={true} />
        {hasResources && isExpanded ? (
          <div ref={treeChildrenBoxRef}>
            {
              props.resourceList.map((resourceUrl: string, idx) => {
                return (
                  <UrlElement
                    key={idx}
                    isHost={false}
                    name={resourceUrl}
                    onExpand={() => measure()}
                  />
                )
              })
            }
          </div>
        ) : null}
      </TreeContents>
    </Tree>
  )
}

export default TreeNode
