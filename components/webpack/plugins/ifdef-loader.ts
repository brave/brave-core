// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import loaderUtils from 'loader-utils';
import path from 'path';

interface OptionObject {
   [key: string]: any;
}

interface Range {
   from: number;
   to: number;
}

/** Holds the line indexes for a complete #if block */
class IfBlock {
   lineIf: number;
   lineEndIf: number;
   elifs: number[];
   lineElse: number | null;
   innerIfs: IfBlock[];

   /**
    * @param line_if Line index of #if
    * @param line_endif Line index of #endif
    * @param elifs Line indexes of #elifs
    * @param line_else Line index of #else, or null
    * @param inner_ifs List of any IfBlocks that are contained within this IfBlock
    */
   constructor(lineIf: number, lineEndIf: number, elifs: number[] = [], lineElse: number | null = null, innerIfs: IfBlock[] = []) {
      this.lineIf = lineIf;
      this.lineEndIf = lineEndIf;
      this.elifs = elifs;
      this.lineElse = lineElse;
      this.innerIfs = innerIfs;
   }

   getIfRange(): Range {
      const to = this.elifs.length > 0 ? this.elifs[0] : this.lineElse != null ? this.lineElse : this.lineEndIf;
      return { from: this.lineIf, to };
   }

   getElifRange(index: number): Range {
      if (this.elifs.length > index) {
         const from = this.elifs[index];
         const to = this.elifs.length > index + 1 ? this.elifs[index + 1] : this.lineElse != null ? this.lineElse : this.lineEndIf;
         return { from, to };
      } else {
         throw new Error(`Invalid elif index '${index}', there are only ${this.elifs.length} elifs`);
      }
   }

   getElseRange(): Range {
      if (this.lineElse != null) {
         return { from: this.lineElse, to: this.lineEndIf };
      } else {
         throw new Error('Cannot use elseRange when elseIx is null');
      }
   }
}

const IfType = { If: 1, Elif: 2 } as const;

let fillCharacter: string;
let uncommentPrefix: string | undefined;

export function parse(source: string, defs: OptionObject, verbose?: boolean, filePath?: string, fillWithBlanks?: boolean, uncommentPrefixString?: string): string {

   if (fillWithBlanks === undefined) fillWithBlanks = false;
   fillCharacter = fillWithBlanks ? ' ' : '/';

   uncommentPrefix = uncommentPrefixString;

   // early skip check: do not process file when no 'if expr="..."' are contained
   if (!source.includes('\u003Cif expr="')) return source;

   const lines = source.split(/\r\n|\n|\r/);

   const ifBlocks = findIfBlocks(lines);
   for (let ifBlock of ifBlocks) {
      applyIf(lines, ifBlock, defs, verbose, filePath);
   }

   return lines.join('\n');
}

function findIfBlocks(lines: string[]): IfBlock[] {
   const blocks: IfBlock[] = [];
   for (let i = 0; i < lines.length; i++) {
      if (matchIf(lines[i])) {
         const ifBlock = parseIfBlock(lines, i);
         blocks.push(ifBlock);
         i = ifBlock.lineEndIf;
      }
   }
   return blocks;
}

/**
 * Parse #if statement at given locatoin
 * @param ifBlockStart Line on which the '#if' is located. (Given line MUST be start of an if-block)
 */
function parseIfBlock(lines: string[], ifBlockStart: number): IfBlock {
   let foundElifs: number[] = [];
   let foundElse: number | null = null;
   let foundEnd: number | undefined;
   let innerIfs: IfBlock[] = [];

   for (let i = ifBlockStart + 1; i < lines.length; i++) {
      const curLine = lines[i];

      const innerIfMatch = matchIf(curLine);
      if (innerIfMatch) {
         const innerIf = parseIfBlock(lines, i);
         innerIfs.push(innerIf);
         i = innerIf.lineEndIf;
         continue;
      }

      const elifMatch = matchIf(curLine, IfType.Elif);
      if (elifMatch) {
         foundElifs.push(i);
         continue;
      }

      const elseMatch = matchElse(curLine);
      if (elseMatch) {
         foundElse = i;
         continue;
      }

      const endMatch = matchEndif(curLine);
      if (endMatch) {
         foundEnd = i;
         break;
      }
   }

   if (foundEnd === undefined) {
      throw new Error(`#if without #endif on line ${ifBlockStart + 1}`);
   }
   return new IfBlock(ifBlockStart, foundEnd, foundElifs, foundElse, innerIfs);
}

const ifRegex = () => /\/\/ \<(if|elif) expr="(.*)"\>$/g;

function matchIf(line: string, type: typeof IfType[keyof typeof IfType] = IfType.If): boolean {
   const re = ifRegex();
   const match = re.exec(line);
   return match !== null && ((type === IfType.If && match[1] === "if") || (type === IfType.Elif && match[1] === "elif"));
}

/**
 * @param line Line to parse, must be a valid \u003Cif expr="..."> statement
 * @returns The if condition
 */
function parseIf(line: string): string {
   const re = ifRegex();
   const match = re.exec(line);
   if (match) {
      return match[2].trim();
   } else {
      throw new Error(`Could not parse \u003Cif expr="...">: '${line}'`);
   }
}

function matchEndif(line: string): boolean {
   const re = /\/\/ <\/if>/g;
   const match = re.exec(line);
   return Boolean(match);
}

function matchElse(line: string): boolean {
   const re = /\/\/ <else>/g;
   const match = re.exec(line);
   return Boolean(match);
}

/** Includes and excludes relevant lines based on evaluation of the provided IfBlock */
function applyIf(lines: string[], ifBlock: IfBlock, defs: OptionObject, verbose: boolean = false, filePath?: string) {
   let includeRange: Range | null = null;

   // gets the condition and parses it
   const ifCond = parseIf(lines[ifBlock.lineIf]);
   const ifRes = evaluate(ifCond, defs);

   const log = (condition: string, outcome: boolean) => {
      if (verbose) {
         console.log(`\u003Cif expr="..."> block lines [${ifBlock.lineIf + 1}-${ifBlock.lineEndIf + 1}]: Condition '${condition}' is ${outcome ? 'TRUE' : 'FALSE'}. ${includeRange != null ? `Including lines [${includeRange.from + 1}-${includeRange.to + 1}]` : 'Excluding everything'} (${filePath})`);
      }
   };

   // finds which part of the #if has to be included, all else is excluded

   if (ifRes) {
      // include the #if body
      includeRange = ifBlock.getIfRange();
      log(ifCond, true);
   } else {
      // if there are #elif checks if one has to be included
      for (let elifIx = 0; elifIx < ifBlock.elifs.length; elifIx++) {
         const elifLine = lines[ifBlock.elifs[elifIx]];
         const elifCond = parseIf(elifLine);
         const elifRes = evaluate(elifCond, defs);
         if (elifRes) {
            // include #elif
            includeRange = ifBlock.getElifRange(elifIx);
            log(elifCond, true);
            break;
         }
      }

      // if no #elif are found then goes to #else branch
      if (includeRange === null) {
         if (ifBlock.lineElse != null) {
            includeRange = ifBlock.getElseRange();
         }
         log(ifCond, false);
      }
   }

   // blanks everything except the part that has to be included
   if (includeRange !== null) {
      blankCode(lines, ifBlock.lineIf, includeRange.from);   // blanks: \u003Cif expr="..."> ... "from"
      blankCode(lines, includeRange.to, ifBlock.lineEndIf);  // blanks: "to" ... </if>
      revealCode(lines, includeRange.from, includeRange.to);  // reveal: "from" ... "to"
   } else {
      blankCode(lines, ifBlock.lineIf, ifBlock.lineEndIf);  // blanks: \u003Cif expr="..."> ... </if>
   }

   // apply to inner #if blocks that have not already been erased
   for (let innerIf of ifBlock.innerIfs) {
      if (includeRange != null && innerIf.lineIf >= includeRange.from && innerIf.lineIf <= includeRange.to) {
         applyIf(lines, innerIf, defs, verbose);
      }
   }
}

/**
 * @return true if block has to be preserved
 */
function evaluate(condition: string, defs: OptionObject): boolean {
   const code = `return (${condition}) ? true : false;`;
   const args = Object.keys(defs);

   let result: boolean;
   try {
      // eslint-disable-next-line no-new-func
      const f = new Function(...args, code);
      result = f(...args.map((k) => defs[k]));
      // console.log(`evaluation of (${condition}) === ${result}`);
   }
   catch (error) {
      throw new Error(`error evaluation \u003Cif expr="..."> condition(${condition}): ${error}`);
   }

   return result;
}

function blankCode(lines: string[], start: number, end: number) {
   for (let t = start; t <= end; t++) {
      const len = lines[t].length;
      const lastChar = lines[t].charAt(len - 1);
      const windowsTermination = lastChar === '\r';
      if (len === 0) {
         lines[t] = '';
      }
      else if (len === 1) {
         lines[t] = windowsTermination ? '\r' : ' ';
      }
      else if (len === 2) {
         lines[t] = windowsTermination ? ' \r' : fillCharacter.repeat(2);
      }
      else {
         lines[t] = windowsTermination ? fillCharacter.repeat(len - 1) + '\r' : fillCharacter.repeat(len);
      }
   }
}

function revealCode(lines: string[], start: number, end: number) {
   // early exit if no prefix is specifed
   if (uncommentPrefix === undefined) return;

   // create a regex capturing the line
   let regex = new RegExp(`^(?<before>\\s*${uncommentPrefix})(?<line>.*)$`);

   // replace lines that match the uncomment prefix
   for (let t = start; t <= end; t++) {
      let r = regex.exec(lines[t]);
      if (r !== null && r.groups !== undefined) {
         lines[t] = " ".repeat(r.groups.before.length) + r.groups.line;
      }
   }
}

export default function (source: string, map) {
   this.cacheable && this.cacheable();

   const options = loaderUtils.getOptions(this) || {};
   const originalData = options.json || options;

   const data = { ...originalData };

   const verboseFlag = "ifdef-verbose";
   const verbose = data[verboseFlag];
   if (verbose !== undefined) {
      delete data[verboseFlag];
   }

   let filePath: string | undefined;
   if (verbose) {
      filePath = path.relative(this.rootContext || '', this.resourcePath);
   }

   const fillWithBlanksFlag = "ifdef-fill-with-blanks";
   const fillWithBlanks = data[fillWithBlanksFlag];
   if (fillWithBlanks !== undefined) {
      delete data[fillWithBlanksFlag];
   }

   const uncommentPrefixFlag = "ifdef-uncomment-prefix";
   const uncommentPrefix = data[uncommentPrefixFlag];
   if (uncommentPrefix !== undefined) {
      delete data[uncommentPrefixFlag];
   }

   try {
      source = parse(source, data, verbose, filePath, fillWithBlanks, uncommentPrefix);
      this.callback(null, source, map);
   } catch (err) {
      const errorMessage = `ifdef-loader error: ${err}`;
      this.callback(new Error(errorMessage));
   }
};
