// TypeScript module declarations for SVG imports
// This allows webpack file-loader to handle SVG imports

declare module '*.svg' {
  const content: string;
  export default content;
}

declare module '*.svg?url' {
  const content: string;
  export default content;
}

declare module '*.svg?inline' {
  const content: React.FunctionComponent<React.SVGAttributes<SVGElement>>;
  export default content;
}

declare module '*.svg?component' {
  const content: React.FunctionComponent<React.SVGAttributes<SVGElement>>;
  export default content;
}