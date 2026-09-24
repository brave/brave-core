// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Maps selected import.meta APIs to CommonJS equivalents for Jest tests.
 * This is not a general ESM compatibility transform: import.meta.resolve is
 * approximated with require.resolve, so built-ins, URL schemes, missing files,
 * ESM export conditions, and the optional parent URL may behave differently.
 */

import type { TsCompilerInstance } from 'ts-jest'
import type ts from 'typescript'

export const name = 'import-meta'
export const version = 1

export function factory({
  configSet,
}: TsCompilerInstance): ts.TransformerFactory<ts.SourceFile> {
  const typescript = configSet.compilerModule

  return (context) => {
    const visit: ts.Visitor = (node) => {
      if (
        typescript.isPropertyAccessExpression(node)
        && typescript.isMetaProperty(node.expression)
        && node.expression.keywordToken === typescript.SyntaxKind.ImportKeyword
      ) {
        switch (node.name.text) {
          case 'main':
            // import.meta.main becomes require.main === module.
            return typescript.factory.createBinaryExpression(
              typescript.factory.createPropertyAccessExpression(
                typescript.factory.createIdentifier('require'),
                'main',
              ),
              typescript.SyntaxKind.EqualsEqualsEqualsToken,
              typescript.factory.createIdentifier('module'),
            )
          case 'url':
            // import.meta.url becomes
            // require('node:url').pathToFileURL(__filename).href.
            return typescript.factory.createPropertyAccessExpression(
              typescript.factory.createCallExpression(
                typescript.factory.createPropertyAccessExpression(
                  typescript.factory.createCallExpression(
                    typescript.factory.createIdentifier('require'),
                    undefined,
                    [typescript.factory.createStringLiteral('node:url')],
                  ),
                  'pathToFileURL',
                ),
                undefined,
                [typescript.factory.createIdentifier('__filename')],
              ),
              'href',
            )
          case 'dirname':
            // import.meta.dirname becomes __dirname.
            return typescript.factory.createIdentifier('__dirname')
          case 'filename':
            // import.meta.filename becomes __filename.
            return typescript.factory.createIdentifier('__filename')
        }
      }

      if (
        typescript.isCallExpression(node)
        && typescript.isPropertyAccessExpression(node.expression)
        && typescript.isMetaProperty(node.expression.expression)
        && node.expression.expression.keywordToken
          === typescript.SyntaxKind.ImportKeyword
        && node.expression.name.text === 'resolve'
      ) {
        // import.meta.resolve(...) becomes
        // require('node:url').pathToFileURL(require.resolve(...)).href.
        return typescript.factory.createPropertyAccessExpression(
          typescript.factory.createCallExpression(
            typescript.factory.createPropertyAccessExpression(
              typescript.factory.createCallExpression(
                typescript.factory.createIdentifier('require'),
                undefined,
                [typescript.factory.createStringLiteral('node:url')],
              ),
              'pathToFileURL',
            ),
            undefined,
            [
              typescript.factory.createCallExpression(
                typescript.factory.createPropertyAccessExpression(
                  typescript.factory.createIdentifier('require'),
                  'resolve',
                ),
                undefined,
                node.arguments,
              ),
            ],
          ),
          'href',
        )
      }

      return typescript.visitEachChild(node, visit, context)
    }

    return (sourceFile) => typescript.visitEachChild(sourceFile, visit, context)
  }
}
