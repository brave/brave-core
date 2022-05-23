#!/usr/bin/env python3

import sys
import clang.cindex
from clang.cindex import CursorKind


def fully_qualified(c):
    if c is None:
        return ''
    elif c.kind == CursorKind.TRANSLATION_UNIT:
        return ''
    else:
        res = fully_qualified(c.semantic_parent)
        if res != '':
            return res + '::' + c.spelling
    return c.spelling


def main():
    index = clang.cindex.Index.create()
    tu = index.parse(sys.argv[1], args='-xc++ --std=c++17'.split())
    print('Translation unit:', tu.spelling)

    for c in tu.cursor.walk_preorder():
        if c.kind == CursorKind.ENUM_DECL or c.kind == CursorKind.ENUM_CONSTANT_DECL:
            print(c.semantic_parent.spelling, '::', c.spelling)


if __name__ == '__main__':
    sys.exit(main())
