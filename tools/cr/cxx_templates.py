# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

TEMPLATES = {
    # A template to give a function an early return, usually to cancel it.
    #
    # e.g.:
    #   void MyClass::MyFunction() {
    #     // Do something
    #   }
    #
    # becomes:
    #   void MyClass::MyFunction() {
    #     if ((true)) return;
    #     // Do something
    #   }
    #
    # Args:
    #   return_type: The return type of the function (e.g. 'void', 'int', etc.)
    #   function_name: The name of the function to be modified.
    #
    # Replacement:
    #   statement: The statement to be used to return (e.g. 'return false;').
    'CancelFunctionUsingEarlyReturn': {
        'pattern': r"""
       (                # Group 1: Capture the function header up to and
                        # including the opening brace
           \b{return_type}\s+
                        # Match the return type at a word boundary followed by
                        # one or more spaces

                        # Match the exact function name
           {function_name}
           \s*\(.*?\)   # Match the parameter list: optional spaces,
                        # literal '(', non-greedy capture of parameters, then
                        # literal ')'
           \s*\{{       # Match optional spaces and then the opening curly
                        # brace 
           \s*\n        # Match any spaces and the following newline character.
       )
       ([ \t]*)         # Group 2: Capture any indentation (spaces or tabs)
                        # from the beginning of the next line
       """,
        'replacement': r'\1\2if ((true)) {statement}\n\2',
        'flags': ['MULTILINE', 'DOTALL', 'VERBOSE'],
    },

    # A template to add the virtual keyword to a function declaration, or
    # virtualise a function.
    #
    # e.g.:
    #   void MyFunction();
    #
    # becomes:
    #   virtual void MyClass::MyFunction();
    #
    # Args:
    #   function_name: The name of the function to be modified.
    'VirtualiseFunction': {
        'pattern': r"""
        ^(?P<indent>\s*)    # Capture any leading indentation.
        (?!virtual\s)       # Do not match if 'virtual' follows the indentation.
        (?P<ret>.*?)        # Non-greedily capture the return type & qualifiers.
        \b(?P<fn>{function_name})\b  # The function name, with word boundaries.
        (?P<args>\s*        # Capture the following as 'args':
            \( .*? \)       #   - Non-greedy capture of the parameter list in
                            #     parentheses.
            \s*             #   - Optional whitespace.
            (?:const\s*)?   #   - Optional trailing 'const'.
            ;               #   - The semicolon ending the declaration.
        )
    """,
        'replacement': r'\g<indent>virtual \g<ret>\g<fn>\g<args>',
        'flags': ['MULTILINE', 'VERBOSE'],
    },

    # A template to make a class a friend of another class.
    #
    # e.g.:
    #   class MyClass {
    #    private:
    #      // Some private members
    #   };
    #
    # becomes:
    #   class MyClass {
    #    private:
    #      friend class MyClass;
    #      // Some private members
    #   };
    #
    # Args:
    #   class_name: The name of the class having friend added to it.
    #
    # Replacement:
    #   class_name: The name of the class to be made a friend.
    'MakeFriendClassOf': {
        'pattern': r"""
        (?sx)                       # Enable DOTALL & VERBOSE modes
        (                           # Begin Group 1: the header of the class
            class\s+{class_name}\b  # Match the 'class' keyword and the given
                                    # class name
            [^{{]*\{{               # Consume characters (until the first curly
                                    # braces)
            .*?                     # Lazily consume the rest of the header
            \n                      # Until we get to a newline
            (?P<indent>[ \t]*)      # Capture the indentation that follows for
                                    # the next line
            private:\s*\n           # Match the private access specifier line
        )
    """,
        'replacement': r'\1\g<indent> friend class {class_name};\n',
        'flags': ['MULTILINE', 'DOTALL', 'VERBOSE'],
    }
}
