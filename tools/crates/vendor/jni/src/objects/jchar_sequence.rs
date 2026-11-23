crate::bind_java_type! {
    pub JCharSequence => "java.lang.CharSequence",
    methods {
        /// Returns the character at the specified index.
        ///
        /// # Throws
        /// - `IndexOutOfBoundsException` - if the index is negative or not less than the length of
        ///   this sequence.
        fn char_at(index: jint) -> jchar,
        /// Returns the length of this character sequence.
        fn length() -> jint,
        /// Returns a new character sequence that is a subsequence of this sequence.
        ///
        /// The subsequence starts with the character at the specified index and ends with the
        /// character at index end - 1.
        ///
        /// # Throws
        /// - `IndexOutOfBoundsException` - if start or end are negative, if end is greater than
        ///   length(), or if start is greater than end.
        fn sub_sequence(start: jint, end: jint) -> JCharSequence,
    }
}
