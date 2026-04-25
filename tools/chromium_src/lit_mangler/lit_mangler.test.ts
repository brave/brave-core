// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { utilsForTest, HTMLTemplateTags } from './lit_mangler'
import ts from 'typescript'

const loadTextForTesting = (text: string) => {
  const sourceFile = ts.createSourceFile(
    'test.ts',
    text,
    ts.ScriptTarget.Latest,
  )
  const template: HTMLTemplateTags = { id: 0, text, children: [] }
  utilsForTest.resetTemplateId()
  utilsForTest.cachePropertyCasesFromText(text)
  utilsForTest.setResult(template)
  utilsForTest.getTemplateLiterals(sourceFile, sourceFile, template)
  utilsForTest.injectPlaceholders(template)
  return template
}

const getOutputForTesting = (template: HTMLTemplateTags) => {
  return utilsForTest.restorePropertyCases(
    utilsForTest.replacePlaceholders(template),
  )
}

describe('Attribute handling', () => {
  it('should not break when attributes are unquoted', () => {
    const template: HTMLTemplateTags = {
      text: '<div ?hidden=${this.foo}>Hello</div>',
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      expect(e.querySelector('div')?.getAttribute('?hidden')).toBe(
        '${this.foo}',
      )
    }, template)
  })

  it('should not break when unquoted attributes have spaces', () => {
    const template: HTMLTemplateTags = {
      text: '<div ?hidden=${this.foo || this.bar}>Hello</div>',
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      expect(e.querySelector('div')?.getAttribute('?hidden')).toBe(
        '${this.foo || this.bar}',
      )
    }, template)
  })

  it('should not break when unquoted attributes have spaces around equals', () => {
    const template: HTMLTemplateTags = {
      text: `<div ?hidden   =  \${this.foo || this.bar}>Hello</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      expect(e.querySelector('div')?.getAttribute('?hidden')).toBe(
        '${this.foo || this.bar}',
      )
    }, template)
  })

  it('should be fine with lit prefixes on attributes', () => {
    const template: HTMLTemplateTags = {
      text: `<div ?hidden   =  \${this.foo || this.bar} .bar=7 bar=12 .baz=\${5 * this.baz} @click=\${this.onClickDoTheThing} data-baz=\${foo} bop=one bad="two" good='seven'>Hello</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const div = e.querySelector('div')
      expect(div).toBeDefined()
      expect(div?.getAttribute('?hidden')).toBe('${this.foo || this.bar}')
      expect(div?.getAttribute('.bar')).toBe('7')
      expect(div?.getAttribute('bar')).toBe('12')
      expect(div?.getAttribute('.baz')).toBe('${5 * this.baz}')
      expect(div?.getAttribute('@click')).toBe('${this.onClickDoTheThing}')
      expect(div?.getAttribute('data-baz')).toBe('${foo}')
      expect(div?.getAttribute('bop')).toBe('one')
      expect(div?.getAttribute('bad')).toBe('two')
      expect(div?.getAttribute('good')).toBe('seven')
    }, template)
  })

  it('should not break when attributes have quotes in them', () => {
    const template: HTMLTemplateTags = {
      text: `<div ?hidden   =  \${this.foo || this.bar || "baz"}>Hello</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const div = e.querySelector('div')
      expect(div).toBeDefined()
      expect(div?.getAttribute('?hidden')).toBe(
        '${this.foo || this.bar || "baz"}',
      )
    }, template)
  })

  it('should not break when attributes have arrows in them', () => {
    const template: HTMLTemplateTags = {
      text: `<div ?hidden   =  \${this.foo || this.bar || "<p>haha</p>"}>Hello</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const div = e.querySelector('div')
      expect(div).toBeDefined()
      expect(div?.getAttribute('?hidden')).toBe(
        '${this.foo || this.bar || "<p>haha</p>"}',
      )
    }, template)
  })

  it('should not break sandbox attribute', () => {
    const template: HTMLTemplateTags = {
      text: `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const iframe = e.querySelector('iframe')
      expect(iframe).toBeDefined()
      expect(iframe?.getAttribute('sandbox')).toBe(
        'allow-scripts allow-same-origin allow-forms allow-popups',
      )
    }, template)

    expect(template.text).toBe(
      `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
    )
  })

  it('should not break sandbox attribute when interpolated', () => {
    const template: HTMLTemplateTags = {
      text: `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups \${this.additionalRestrictions}" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const iframe = e.querySelector('iframe')
      expect(iframe).toBeDefined()
      expect(iframe?.getAttribute('sandbox')).toBe(
        'allow-scripts allow-same-origin allow-forms allow-popups ${this.additionalRestrictions}',
      )
    }, template)

    expect(template.text).toBe(
      `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups \${this.additionalRestrictions}" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
    )
  })

  it('should not break sandbox attribute using ternary', () => {
    const template: HTMLTemplateTags = {
      text: `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups \${this.additionalRestrictions ? "allow-foo" : ""}" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const iframe = e.querySelector('iframe')
      expect(iframe).toBeDefined()
      expect(iframe?.getAttribute('sandbox')).toBe(
        'allow-scripts allow-same-origin allow-forms allow-popups ${this.additionalRestrictions ? "allow-foo" : ""}',
      )
    }, template)

    expect(template.text).toBe(
      `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups \${this.additionalRestrictions ? "allow-foo" : ""}" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
    )
  })

  it('should not break sandbox attribute using ternary and mixed quotes', () => {
    const template: HTMLTemplateTags = {
      text: `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups \${this.additionalRestrictions ? "allow-foo" : ''}" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const iframe = e.querySelector('iframe')
      expect(iframe).toBeDefined()
      expect(iframe?.getAttribute('sandbox')).toBe(
        'allow-scripts allow-same-origin allow-forms allow-popups ${this.additionalRestrictions ? "allow-foo" : \'\'}',
      )
    }, template)

    expect(template.text).toBe(
      `<iframe class="wp-embedded-content" sandbox="allow-scripts allow-same-origin allow-forms allow-popups \${this.additionalRestrictions ? "allow-foo" : ''}" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" width="500" height="282" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>`,
    )
  })

  it("doesn't mangle meta tags by default", () => {
    const template: HTMLTemplateTags = {
      text: `<meta name="viewport" content="width=device-width, initial-scale=\${this.scale}">`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle(() => {}, template)
    expect(template.text).toBe(
      `<meta name="viewport" content="width=device-width, initial-scale=\${this.scale}">`,
    )
  })

  it('can mangle meta tags when specified', () => {
    const template: HTMLTemplateTags = {
      text: `<meta name="viewport" content="width=device-width, initial-scale=\${this.scale}">`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((e) => {
      const meta = e.querySelector('meta')
      expect(meta).toBeDefined()
      meta?.setAttribute(
        'content',
        'width=device-width, initial-scale=${this.scale * 2}',
      )
    }, template)
    expect(template.text).toBe(
      `<meta name="viewport" content="width=device-width, initial-scale=\${this.scale * 2}">`,
    )
  })
})

describe('Property handling', () => {
  it('property names should roundtrip through the mangler', () => {
    const original = `<div .fooBar="\${this.foo}" .baz="1" .hElLo="2">Hello</div>`
    const template = loadTextForTesting(original)

    utilsForTest.mangleAll(
      (e) => {
        // don't do anything, we're just checking the output is sane
      },
      () => true,
    )

    expect(getOutputForTesting(template)).toBe(
      `<div .fooBar="\${this.foo}" .baz="1" .hElLo="2">Hello</div>`,
    )
  })

  it('should still lowercase attributes', () => {
    const original = `<div .fooBar="\${this.foo}" .baz="1" .hElLo="2" BAZ="7" ?hiDDen="true">Hello</div>`
    const template = loadTextForTesting(original)

    utilsForTest.mangleAll(
      (e) => {
        // don't do anything, we're just checking the output is sane
      },
      () => true,
    )

    expect(getOutputForTesting(template)).toBe(original)
  })

  it('should handle camelCase properties in children', () => {
    const original = `<div .fooBar="\${this.foo}" .baz="1" .hElLo="2">
                <span .propertyName="Hello">World</span>
            </div>`
    const template = loadTextForTesting(original)
    utilsForTest.mangle((e) => {
      // don't do anything, we're just checking the output is sane
    }, template)

    expect(getOutputForTesting(template)).toBe(original)
  })

  it('should handle camelCase properties in nested templates', () => {
    const original = `<div .fooBar="\${this.foo}" .baz="1" .hElLo="2">
                \${items.map(item => html\`<span .propertyName="\${item.value}">World</span>\`)}
            </div>`
    const template = loadTextForTesting(original)

    // Run a mangler over all templates
    utilsForTest.mangleAll(
      (e) => {
        // don't do anything, we're just checking the output is sane
      },
      () => true,
    )

    expect(getOutputForTesting(template)).toBe(original)
  })

  it('not running any manglers should not change the output', () => {
    const original = `<div .fooBar="\${this.foo}" .baz="1" .hElLo="2">
                \${items.map(item => html\`<span .propertyName="\${item.value}">World</span>\`)}
            </div>`
    const template = loadTextForTesting(original)
    expect(getOutputForTesting(template)).toBe(original)
  })
})

describe('Escaping', () => {
  it('should escape quotes', () => {
    const template: HTMLTemplateTags = {
      text: `<div attr=\${"foo"}>"\${this.foo}"</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle(() => {}, template)
    expect(template.text).toBe(`<div attr="\${"foo"}">"\${this.foo}"</div>`)
  })

  it('should escape script tags in literals', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"<script>alert('pwnd')</script>"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((t) => {
      expect(t.querySelector('script')).toBeNull()
    }, template)
    expect(template.text).toBe(
      `<div>\${"<script>alert('pwnd')</script>"}</div>`,
    )
  })

  it('should escape tags in literals', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"<h1>Jay</h1>"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)

    utilsForTest.mangle((t) => {
      expect(t.querySelector('h1')).toBeNull()
    }, template)
    expect(template.text).toBe(`<div>\${"<h1>Jay</h1>"}</div>`)
  })

  it('should be fine with escaped HTML entities', () => {
    const template: HTMLTemplateTags = {
      text: `<div>&lt;script&gt;alert('hahaha')&lt;script&gt;</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(
      `<div>&lt;script&gt;alert('hahaha')&lt;script&gt;</div>`,
    )
  })

  it('should be fine with escaped HTML entities in interpolated strings', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"&lt;script&gt;alert('hahaha')&lt;script&gt;"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(
      `<div>\${"&lt;script&gt;alert('hahaha')&lt;script&gt;"}</div>`,
    )
  })

  it('should be fine with escaped HTML entities in interpolated strings', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"&lt;script&gt;alert('hahaha')&lt;script&gt;"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(
      `<div>\${"&lt;script&gt;alert('hahaha')&lt;script&gt;"}</div>`,
    )
  })

  it('should be fine with unescaped HTML entities in interpolated strings', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"<foo&"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(`<div>\${"<foo&"}</div>`)
  })

  it('should be fine with unescaped tags in interpolated strings', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"<script>alert('pwnd')</script>"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(
      `<div>\${"<script>alert('pwnd')</script>"}</div>`,
    )
  })

  it('should be fine with unescaped tags and entities in interpolated strings', () => {
    const template: HTMLTemplateTags = {
      text: `<div>\${"$lt;<script$gt;>alert('pwnd')&lt;</script&gt;>"}</div>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(
      `<div>\${"$lt;<script$gt;>alert('pwnd')&lt;</script&gt;>"}</div>`,
    )
  })

  it('should be fine with quotes in attributes', () => {
    const template: HTMLTemplateTags = {
      text: `<img src=\${"foo onload='javascript:alert(\`pwnd\`)'"}>`,
      children: [],
      id: 0,
    }
    utilsForTest.setResult(template)
    utilsForTest.mangle((t) => {}, template)
    expect(template.text).toBe(
      `<img src="\${"foo onload='javascript:alert(\`pwnd\`)'"}">`,
    )
  })
})

describe('End to end', () => {
  const exampleHtml = `import { html } from "lit-html";
import { FakeElement } from "../types/element";

export function getHtml(this: FakeElement & { name: string }) {
    return html\`<div class="\${this.data.name} greeter" data-name=\${this.data.name}>
        Hello <span>\${this.name}</span>
    </div>\`;
}

// This one is particularly brutal 'cause we don't have a component boundary to mangle
export function getList(this: string[]) {
    const haxor = 'whatevs" onload="javascript:alert(\`pwnd\`)'
    return html\`<div ?hidden   = \${this.length === 0 && "foo"} .count=\${this.length} @click=\${console.log} data-haxor='\${haxor}'>
        Hi All!

        Greetings could be: \${["Hi", 'Kiora', 'Gidday'].map(g => html\`<b>\${g}</b>\`).join('\n')}

        <div class="greetings count-\${this.length}">
            \${this.map(t => getHtml.call({ name: t, data: {} }))}
        </div>
    </div>\`;
}
`
  const sourceFile = ts.createSourceFile(
    'test.ts',
    exampleHtml,
    ts.ScriptTarget.Latest,
  )

  it('should be able to load the template', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)

    expect(result.children.length).toBe(2)
    expect(result.children[0].children.length).toBe(0)
    expect(result.children[1].children.length).toBe(1)

    expect(result.children[0].text)
      .toBe(`<div class="\${this.data.name} greeter" data-name=\${this.data.name}>
        Hello <span>\${this.name}</span>
    </div>`)
    expect(result.children[1].text)
      .toBe(`<div ?hidden   = \${this.length === 0 && "foo"} .count=\${this.length} @click=\${console.log} data-haxor='\${haxor}'>
        Hi All!

        Greetings could be: \${["Hi", 'Kiora', 'Gidday'].map(g => html\`<b>\${g}</b>\`).join('\n')}

        <div class="greetings count-\${this.length}">
            \${this.map(t => getHtml.call({ name: t, data: {} }))}
        </div>
    </div>`)
    expect(result.children[1].children[0].text).toBe(`<b>\${g}</b>`)
  })

  it('should be able to inject placeholders', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)

    expect(result.text).toBe(`import { html } from "lit-html";
import { FakeElement } from "../types/element";

export function getHtml(this: FakeElement & { name: string }) {
    return html\`$$lit_mangler_1$$\`;
}

// This one is particularly brutal 'cause we don't have a component boundary to mangle
export function getList(this: string[]) {
    const haxor = 'whatevs" onload="javascript:alert(\`pwnd\`)'
    return html\`$$lit_mangler_2$$\`;
}
`)

    expect(result.children.length).toBe(2)
    expect(result.children[0].children.length).toBe(0)
    expect(result.children[1].children.length).toBe(1)

    expect(result.children[0].text)
      .toBe(`<div class="\${this.data.name} greeter" data-name=\${this.data.name}>
        Hello <span>\${this.name}</span>
    </div>`)
    expect(result.children[1].text)
      .toBe(`<div ?hidden   = \${this.length === 0 && "foo"} .count=\${this.length} @click=\${console.log} data-haxor='\${haxor}'>
        Hi All!

        Greetings could be: \${["Hi", 'Kiora', 'Gidday'].map(g => html\`$$lit_mangler_3$$\`).join('\n')}

        <div class="greetings count-\${this.length}">
            \${this.map(t => getHtml.call({ name: t, data: {} }))}
        </div>
    </div>`)
    expect(result.children[1].children[0].text).toBe(`<b>\${g}</b>`)
  })

  it('should mangle the first template if none is specified', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)
    utilsForTest.setResult(result)
    utilsForTest.mangle((element) => {
      const root = element.querySelector('div')
      expect(root).toBeDefined()
      expect(root?.getAttribute('class')).toBe('${this.data.name} greeter')
    })
  })

  it('should be possible to select a template by predicate', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)
    utilsForTest.mangle(
      (element) => {
        expect(element.querySelector('b')?.outerHTML).toBe('<b>${g}</b>')
      },
      (t) => t.text.startsWith('<b>'),
    )
  })

  it('should be possible to pass in a template to mangle', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)
    utilsForTest.mangle((element) => {
      expect(element.querySelector('div')?.textContent).toContain('Hi All!')
    }, result.children[1])
  })

  it('should throw an error if no template is found', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)
    expect(() =>
      utilsForTest.mangle(
        () => {},
        () => false,
      ),
    ).toThrow()
  })

  it('should be possible to mangle the template', () => {
    const result: HTMLTemplateTags = { id: 0, text: exampleHtml, children: [] }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)
    utilsForTest.setResult(result)

    utilsForTest.mangle(
      (e) => {
        const root = e.querySelector('div')
        root?.removeAttribute('?hidden')
        root!.childNodes[0].textContent =
          root!.childNodes[0].textContent!.replace('        Hi All!\n', '')
        root!.innerHTML =
          `<div>Greetings from the Mangler</div>` + root!.innerHTML
      },
      (t) => t.text.includes('Hi All!'),
    )

    utilsForTest.mangle(
      (e) => {
        e.querySelector('b')?.setAttribute('hidden', '${g === "hi"}')
      },
      (t) => t.text.startsWith('<b'),
    )

    utilsForTest.mangle((e) => {
      const root = e.querySelector('div')
      root?.setAttribute('class', '${this.name === "jay" && "best"}')
    })

    const output = utilsForTest.replacePlaceholders(result)
    expect(output).toBe(`import { html } from "lit-html";
import { FakeElement } from "../types/element";

export function getHtml(this: FakeElement & { name: string }) {
    return html\`<div class="\${this.name === "jay" && "best"}" data-name="\${this.data.name}">
        Hello <span>\${this.name}</span>
    </div>\`;
}

// This one is particularly brutal 'cause we don't have a component boundary to mangle
export function getList(this: string[]) {
    const haxor = 'whatevs" onload="javascript:alert(\`pwnd\`)'
    return html\`<div .count="\${this.length}" @click="\${console.log}" data-haxor="\${haxor}"><div>Greetings from the Mangler</div>

        Greetings could be: \${["Hi", 'Kiora', 'Gidday'].map(g => html\`<b hidden="\${g === "hi"}">\${g}</b>\`).join('\n')}

        <div class="greetings count-\${this.length}">
            \${this.map(t => getHtml.call({ name: t, data: {} }))}
        </div>
    </div>\`;
}
`)
  })

  it('should be possible to mangle all matching templates', () => {
    const result: HTMLTemplateTags = {
      id: 0,
      text: exampleHtml,
      children: [
        { id: 1, text: '<div>Hello</div>', children: [] },
        { id: 2, text: '<div>Hello</div>', children: [] },
      ],
    }
    utilsForTest.resetTemplateId()
    utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
    utilsForTest.injectPlaceholders(result)
    utilsForTest.setResult(result)

    utilsForTest.mangleAll(
      (e) => {
        e.querySelector('div')!.textContent = 'World'
      },
      (t) => t.text.includes('Hello'),
    )

    for (const template of utilsForTest.findTemplates((t) =>
      t.text.includes('Hello'),
    )) {
      expect(template.text).toBe('<div>World</div>')
    }
  })

  it('should handle HTML files with Lit templates', () => {
    utilsForTest.setResult(undefined)
    utilsForTest.load('test/data/lit_mangle_html.html')

    utilsForTest.mangle(
      (e) => e.querySelector('div').setAttribute('foo', 'bar'),
      (t) => t.text.includes('div'),
    )
    utilsForTest.mangle(
      (e) => e.querySelector('span').setAttribute('bar', 'foo'),
      (t) => t.text.includes('span'),
    )

    const result = utilsForTest.replacePlaceholders(utilsForTest.getResult()!)
    expect(result).toBe(`<div foo="bar">
  \${items.map(i => html\`<span bar="foo">\${i}</span>\`)}
</div>
`)
  })
})
