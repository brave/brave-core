const path = require('path');

module.exports = {
  braveResolveId: function (source, origin, params, funcs) {
    const {srcPath, genPath, excludes} = params;
    const {relativePath, joinPaths, combinePaths} = funcs;

    const msg = '\n----------------\n' +
                'braveResolveId:\n\tsrcPath: [' +
                srcPath +
                '],\n\tgenPath: [' +
                genPath +
                '],\n\tsource: [' +
                source +
                '],\n\torigin: [' +
                origin +
                ']\n';
    process.stderr.write(msg);

    const chromeResourcesUrl = 'chrome://resources/';
    const schemeRelativeResourcesUrl = '//resources/';
    const braveResourcesUrl = 'chrome://brave-resources/';
    const braveSchemeRelativeResourcesUrl = '//brave-resources/';
    const resourcesPreprocessedPath = 'ui/webui/resources/preprocessed/';

    // sources not referencing `brave-resources`
    if (source.startsWith(chromeResourcesUrl) ||
        source.startsWith(schemeRelativeResourcesUrl) ||
        (!!origin && origin.startsWith(resourcesPreprocessedPath) &&
         source.indexOf(braveResourcesUrl) === -1 &&
         source.indexOf(braveSchemeRelativeResourcesUrl) === -1))
    {
      return undefined;
    }

    // sources referencing `brave-resources`
    const braveResourcesSrcPath = joinPaths(srcPath, 'brave/ui/webui/resources/')
    const braveResourcesGenPath = joinPaths(genPath, 'brave/ui/webui/resources/')
    let pathFromBraveResources = ''
    if (source.startsWith(braveResourcesUrl)) {
      pathFromBraveResources = source.slice(braveResourcesUrl.length)
    } else if (source.startsWith(braveSchemeRelativeResourcesUrl)) {
      pathFromBraveResources = source.slice(braveSchemeRelativeResourcesUrl.length)
    } else if (!!origin && origin.startsWith(braveResourcesSrcPath)) {
      pathFromBraveResources = combinePaths(relativePath(braveResourcesSrcPath, origin), source);
    } else if (!!origin && origin.startsWith(braveResourcesGenPath)) {
      pathFromBraveResources = combinePaths(relativePath(braveResourcesGenPath, origin), source);
    } else {
      return undefined;
    }
    // avoid excludes and non-generated files
    const fullPath = chromeResourcesUrl + pathFromBraveResources;
    if (excludes.includes(fullPath)) {
      return {id: fullPath, external: true};
    }
    // JS compiled into gen directory
    if (pathFromBraveResources.endsWith('.js')) {
      return joinPaths(braveResourcesGenPath, pathFromBraveResources);
    }
    return joinPaths(braveResourcesSrcPath, pathFromBraveResources);
  }
}
