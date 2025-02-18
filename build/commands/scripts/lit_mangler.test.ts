// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { utilsForTest, HTMLTemplateTags } from './lit_mangler'
import ts from 'typescript'

describe('Attribute handling', () => {
    it('should not break when attributes are unquoted', () => {
        const template: HTMLTemplateTags = { text: '<div ?hidden=${this.foo}>Hello</div>', children: [], id: 0 }
        utilsForTest.setResult(template)

        utilsForTest.mangle(e => {
            expect(e.querySelector('div')?.getAttribute('?hidden')).toBe('${this.foo}')
        }, template)
    })

    it('should not break when unquoted attributes have spaces', () => {
        const template: HTMLTemplateTags = { text: '<div ?hidden=${this.foo || this.bar}>Hello</div>', children: [], id: 0 }
        utilsForTest.setResult(template)

        utilsForTest.mangle(e => {
            expect(e.querySelector('div')?.getAttribute('?hidden')).toBe('${this.foo || this.bar}')
        }, template)
    })

    it('should not break when unquoted attributes have spaces around equals', () => {
        const template: HTMLTemplateTags = { text: `<div ?hidden   =  \${this.foo || this.bar}>Hello</div>`, children: [], id: 0 }
        utilsForTest.setResult(template)

        utilsForTest.mangle(e => {
            expect(e.querySelector('div')?.getAttribute('?hidden')).toBe('${this.foo || this.bar}')
        }, template)
    })

    it('should be fine with lit prefixes on attributes', () => {
        const template: HTMLTemplateTags = { text: `<div ?hidden   =  \${this.foo || this.bar} .bar=7 bar=12 .baz=\${5 * this.baz} @click=\${this.onClickDoTheThing} data-baz=\${foo} bop=one bad="two" good='seven'>Hello</div>`, children: [], id: 0 }
        utilsForTest.setResult(template)

        utilsForTest.mangle(e => {
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
        const template: HTMLTemplateTags = { text: `<div ?hidden   =  \${this.foo || this.bar || "baz"}>Hello</div>`, children: [], id: 0 }
        utilsForTest.setResult(template)

        utilsForTest.mangle(e => {
            const div = e.querySelector('div')
            expect(div).toBeDefined()
            expect(div?.getAttribute('?hidden')).toBe('${this.foo || this.bar || "baz"}')
        }, template)
    })

    it('should not break when attributes have arrows in them', () => {
        const template: HTMLTemplateTags = { text: `<div ?hidden   =  \${this.foo || this.bar || "<p>haha</p>"}>Hello</div>`, children: [], id: 0 }
        utilsForTest.setResult(template)

        utilsForTest.mangle(e => {
            const div = e.querySelector('div')
            expect(div).toBeDefined()
            expect(div?.getAttribute('?hidden')).toBe('${this.foo || this.bar || "<p>haha</p>"}')
        }, template)
    })
    
});

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
    const sourceFile = ts.createSourceFile('test.ts', exampleHtml, ts.ScriptTarget.Latest)

    it('should be able to load the template', () => {
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
        utilsForTest.resetTemplateId()
        utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)


        expect(result.children.length).toBe(2)
        expect(result.children[0].children.length).toBe(0)
        expect(result.children[1].children.length).toBe(1)

        expect(result.children[0].text).toBe(`<div class="\${this.data.name} greeter" data-name=\${this.data.name}>
        Hello <span>\${this.name}</span>
    </div>`)
        expect(result.children[1].text).toBe(`<div ?hidden   = \${this.length === 0 && "foo"} .count=\${this.length} @click=\${console.log} data-haxor='\${haxor}'>
        Hi All!

        Greetings could be: \${["Hi", 'Kiora', 'Gidday'].map(g => html\`<b>\${g}</b>\`).join('\n')}
        
        <div class="greetings count-\${this.length}">
            \${this.map(t => getHtml.call({ name: t, data: {} }))}
        </div>
    </div>`)
        expect(result.children[1].children[0].text).toBe(`<b>\${g}</b>`)
    })

    it('should be able to inject placeholders', () => {
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
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

        expect(result.children[0].text).toBe(`<div class="\${this.data.name} greeter" data-name=\${this.data.name}>
        Hello <span>\${this.name}</span>
    </div>`)
        expect(result.children[1].text).toBe(`<div ?hidden   = \${this.length === 0 && "foo"} .count=\${this.length} @click=\${console.log} data-haxor='\${haxor}'>
        Hi All!

        Greetings could be: \${["Hi", 'Kiora', 'Gidday'].map(g => html\`$$lit_mangler_3$$\`).join('\n')}
        
        <div class="greetings count-\${this.length}">
            \${this.map(t => getHtml.call({ name: t, data: {} }))}
        </div>
    </div>`)
        expect(result.children[1].children[0].text).toBe(`<b>\${g}</b>`)
    })

    it('should mangle the first template if none is specified', () => {
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
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
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
        utilsForTest.resetTemplateId()
        utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
        utilsForTest.injectPlaceholders(result)
        utilsForTest.mangle((element) => {
            expect(element.querySelector('b')?.outerHTML).toBe('<b>\${g}</b>')
        }, t => t.text.startsWith('<b>'))
    })

    it('should be possible to pass in a template to mangle', () => {
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
        utilsForTest.resetTemplateId()
        utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
        utilsForTest.injectPlaceholders(result)
        utilsForTest.mangle((element) => {
            expect(element.querySelector('div')?.textContent).toContain('Hi All!')
        }, result.children[1])
    })

    it('should throw an error if no template is found', () => {
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
        utilsForTest.resetTemplateId()
        utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
        utilsForTest.injectPlaceholders(result)
        expect(() => utilsForTest.mangle(() => { }, () => false)).toThrow()
    })

    it('should be possible to mangle the template', () => {
        const result: HTMLTemplateTags = { text: exampleHtml, children: [] }
        utilsForTest.resetTemplateId()
        utilsForTest.getTemplateLiterals(sourceFile, sourceFile, result)
        utilsForTest.injectPlaceholders(result)
        utilsForTest.setResult(result)

        utilsForTest.mangle(e => {
            const root = e.querySelector('div')
            root?.removeAttribute('?hidden')
            root!.childNodes[0].textContent = root!.childNodes[0].textContent!.replace("Hi All!", '')
            root?.prepend(`<div>Greetings from the Mangler</div>`)
        }, t => t.text.includes('Hi All!'))

        utilsForTest.mangle(e => {
            e.querySelector('b')?.setAttribute('hidden', '${g === "hi"}')
        }, t => t.text.startsWith('<b'))
        
        utilsForTest.mangle(e => {
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
})
