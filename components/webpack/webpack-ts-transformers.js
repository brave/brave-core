const TypescriptPluginStyledComponents = require('typescript-plugin-styled-components')
const createStyledComponentsTransformer = TypescriptPluginStyledComponents.default
const styledComponentsTransformer = createStyledComponentsTransformer()
module.exports = () => ({
  before: [
    styledComponentsTransformer
  ]
})
