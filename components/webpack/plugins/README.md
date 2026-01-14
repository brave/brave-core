# Webpack Plugins

## ifdef-loader

A webpack loader for conditional compilation Typescript files using GRIT-style `<if expr="...">` syntax.

### Overview

The `ifdef-loader` enables conditional compilation in webpack-bundled code using the same syntax as Chromium's GRIT preprocessing system. This allows you to include or exclude code blocks based on build-time configuration flags.

### Syntax

The loader supports GRIT-style conditional directives in JavaScript/TypeScript comments:

```typescript
// <if expr="DEBUG">
console.log('Debug mode enabled');
// </if>

// <if expr="is_ios">
import { IOSModule } from './ios';
// <elif expr="is_android">
import { AndroidModule } from './android';
// <else>
import { WebModule } from './web';
// </if>
```

### Supported Directives

- **`<if expr="...">`** - Conditional block that evaluates the expression
- **`<elif expr="...">`** - Alternative condition (can have multiple)
- **`<else>`** - Fallback when all conditions are false
- **`</if>`** - Closes the conditional block

### Expression Syntax

Available variables are defined in components/webpack:build_flags_json

```typescript
// Boolean expressions
// <if expr="DEBUG && VERBOSE">

// Comparisons
// <if expr="VERSION >= 2">

// String matching
// <if expr="PLATFORM === 'ios'">

// Complex logic
// <if expr="PROD || STAGING">
// <if expr="!DISABLED">
```

### Related Tools in Chromium

This loader implements similar functionality to Chromium's preprocessing tools.

- **[GRIT (Google Resource and Internationalization Tool)](https://dev.chromium.org/developers/tools-we-use-in-chromium/grit)** - Chromium's resource and internationalization system that uses `<if>` expressions in XML files
- **[preprocess_if_expr.py](/tools/grit/preprocess_if_expr.py)** - Python script that processes `<if expr>` directives in HTML/JS/TS(commit [7382b6d](https://github.com/nippur72/ifdef-loader/commit/7382b6d36842781a0b5a691e09d9af4cfe9df30f)) files using GRIT's evaluation engine

The `ifdef-loader` provides webpack-native conditional compilation without requiring Python preprocessing, making it suitable for pure JavaScript/TypeScript build pipelines.

### Upstream Source

This implementation is based on [ifdef-loader](https://github.com/nippur72/ifdef-loader) with modifications for:

- GRIT-style `<if expr="...">` syntax
- removal of features we don't need in Chromium

