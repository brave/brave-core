const path = require('path');

module.exports = {
  braveResolveId: function (params, source, origin, relativePath, joinPaths, combinePaths, chromeResourcesUrl, nonGeneratedFiles) {
    const {srcPath, genPath, excludes} = params;
    const resourcesSrcPath = joinPaths(srcPath, 'ui/webui/resources/');
    const resourcesGenPath = joinPaths(genPath, 'ui/webui/resources/');
    const braveResourcesUrl = 'chrome://brave-resources/';
    // sources not referencing `brave-resources`
    if (source.startsWith(chromeResourcesUrl) ||
       (!!origin && origin.startsWith(resourcesSrcPath) && source.indexOf(braveResourcesUrl) === -1) ||
       (!!origin && origin.startsWith(resourcesGenPath) && source.indexOf(braveResourcesUrl) === -1))
    {
      return undefined;
    }
    // sources referencing `brave-resources`
    const braveResourcesSrcPath = joinPaths(srcPath, 'brave/ui/webui/resources/')
    const braveResourcesGenPath = joinPaths(genPath, 'brave/ui/webui/resources/')
    let pathFromBraveResources = ''
    if (source.startsWith(braveResourcesUrl)) {
      pathFromBraveResources = source.slice(braveResourcesUrl.length)
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
    const filename = path.basename(source);
    if (nonGeneratedFiles.includes(filename)) {
      return joinPaths(resourcesSrcPath, pathFromBraveResources);
    }
    // JS compiled into gen directory
    if (pathFromBraveResources.endsWith('.js')) {
      return joinPaths(braveResourcesGenPath, pathFromBraveResources);
    }
    return joinPaths(braveResourcesSrcPath, pathFromBraveResources);
  }
}
