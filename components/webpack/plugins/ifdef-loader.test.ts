// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { parse } from './ifdef-loader';

describe('ifdef-loader', () => {
  describe('basic conditionals', () => {
    it('should include code when condition is true', () => {
      const source = `const foo = 1;
// <if expr="DEBUG">
const debug = true;
console.log('debug');
// </if>
const bar = 2;`;

      const result = parse(source, { DEBUG: true });

      expect(result).toBe(`const foo = 1;

const debug = true;
console.log('debug');

const bar = 2;`);
    });

    it('should exclude code when condition is false', () => {
      const source = `const foo = 1;
// <if expr="DEBUG">
const debug = true;
// </if>
const bar = 2;`;

      const result = parse(source, { DEBUG: false });

      expect(result).toBe(`const foo = 1;



const bar = 2;`);
    });

    it('should handle if/else branches', () => {
      const source = `// <if expr="PROD">
const env = 'production';
// <else>
const env = 'development';
// </if>`;

      expect(parse(source, { PROD: true })).toBe(`
const env = 'production';


`);

      expect(parse(source, { PROD: false })).toBe(`


const env = 'development';
`);
    });

    it('should handle if/elif/else chains', () => {
      const source = `// <if expr="ENV === 'prod'">
const config = 'production';
// <elif expr="ENV === 'staging'">
const config = 'staging';
// <else>
const config = 'development';
// </if>`;

      expect(parse(source, { ENV: 'prod' })).toContain("'production'");
      expect(parse(source, { ENV: 'staging' })).toContain("'staging'");
      expect(parse(source, { ENV: 'dev' })).toContain("'development'");
    });
  });

  describe('nested conditionals', () => {
    it('should handle nested if blocks', () => {
      const source = `const start = 1;
// <if expr="OUTER">
const outer = true;
// <if expr="INNER">
const inner = true;
// </if>
const afterInner = true;
// </if>
const end = 1;`;

      const withBoth = parse(source, { OUTER: true, INNER: true });
      expect(withBoth).toContain('outer = true');
      expect(withBoth).toContain('inner = true');
      expect(withBoth).toContain('afterInner = true');

      const outerOnly = parse(source, { OUTER: true, INNER: false });
      expect(outerOnly).toContain('outer = true');
      expect(outerOnly).not.toContain('inner = true');
      expect(outerOnly).toContain('afterInner = true');

      const noneIncluded = parse(source, { OUTER: false, INNER: true });
      expect(noneIncluded).not.toContain('outer');
      expect(noneIncluded).not.toContain('inner');
    });
  });

  describe('expressions', () => {
    it('should evaluate boolean operators', () => {
      const source = `// <if expr="DEBUG && VERBOSE">
const log = true;
// </if>`;

      expect(parse(source, { DEBUG: true, VERBOSE: true })).toContain('log = true');
      expect(parse(source, { DEBUG: true, VERBOSE: false })).not.toContain('log = true');
      expect(parse(source, { DEBUG: false, VERBOSE: true })).not.toContain('log = true');
    });

    it('should evaluate comparisons', () => {
      const source = `// <if expr="VERSION >= 2">
const newFeature = true;
// </if>
// <if expr="PLATFORM === 'ios'">
const isIOS = true;
// </if>`;

      const v3 = parse(source, { VERSION: 3, PLATFORM: 'ios' });
      expect(v3).toContain('newFeature = true');
      expect(v3).toContain('isIOS = true');

      const v1 = parse(source, { VERSION: 1, PLATFORM: 'android' });
      expect(v1).not.toContain('newFeature = true');
      expect(v1).not.toContain('isIOS = true');
    });
  });

  describe('edge cases', () => {
    it('should return source unchanged when no if expressions present', () => {
      const source = `const foo = 1;
const bar = 2;`;

      expect(parse(source, { DEBUG: true })).toBe(source);
    });

    it('should throw error for unclosed if block', () => {
      const source = `// <if expr="DEBUG">
const debug = true;`;

      expect(() => parse(source, { DEBUG: true })).toThrow('#if without #endif');
    });

    it('should throw error for invalid expressions', () => {
      const source = `// <if expr="INVALID SYNTAX">
const foo = 1;
// </if>`;

      expect(() => parse(source, {})).toThrow('error evaluation');
    });

    it('should preserve line numbers for source maps', () => {
      const source = `line1
// <if expr="DEBUG">
line3
line4
// </if>
line6`;

      const result = parse(source, { DEBUG: false });
      const lines = result.split('\n');

      expect(lines.length).toBe(6);
      expect(lines[0]).toBe('line1');
      expect(lines[1]).toBe('');
      expect(lines[2]).toBe('');
      expect(lines[3]).toBe('');
      expect(lines[4]).toBe('');
      expect(lines[5]).toBe('line6');
    });
  });

  describe('real-world usage', () => {
    it('should handle platform-specific code', () => {
      const source = `// <if expr="is_ios">
import { IOSModule } from './ios';
// <elif expr="is_android">
import { AndroidModule } from './android';
// <else>
import { WebModule } from './web';
// </if>

export const PlatformModule =
// <if expr="is_ios">
  IOSModule;
// <elif expr="is_android">
  AndroidModule;
// <else>
  WebModule;
// </if>`;

      const iosResult = parse(source, { is_ios: true, is_android: false });
      expect(iosResult).toContain('IOSModule');
      expect(iosResult).not.toContain('AndroidModule');
      expect(iosResult).not.toContain('WebModule');

      const androidResult = parse(source, { is_ios: false, is_android: true });
      expect(androidResult).not.toContain('IOSModule');
      expect(androidResult).toContain('AndroidModule');
      expect(androidResult).not.toContain('WebModule');

      const webResult = parse(source, { is_ios: false, is_android: false });
      expect(webResult).not.toContain('IOSModule');
      expect(webResult).not.toContain('AndroidModule');
      expect(webResult).toContain('WebModule');
    });
  });
});
