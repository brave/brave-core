crate::bind_java_type! {
    pub JCollection => "java.util.Collection",
    methods {
        /// Adds the given element to this set if it is not already present
        ///
        /// Returns `true` if the collection was modified. Returns false if the collection already contains the element and
        /// the collection doesn't allow duplicates.
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the add operation is not supported
        /// - `ClassCastException` - if the element type isn't compatible with the collection
        /// - `NullPointerException` - if the given element is null and the collection does not allow null values
        /// - `IllegalArgumentException` - if the element has a property that prevents it from being added to this collection
        /// - `IllegalStateException` - if the element cannot be added due to the current state of the collection
        fn add(element: JObject) -> bool,

        /// Removes the given element from this collection if it is present
        ///
        /// Returns true if the element was contained in the collection and removed, false otherwise.
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the remove operation is not supported
        /// - `ClassCastException` - if the element type isn't compatible with the collection
        /// - `NullPointerException` - if the given element is null and the collection does not allow null values
        fn remove(element: JObject) -> bool,

        /// Removes all of the elements from this collection.
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the clear operation is not supported
        fn clear() -> (),

        /// Checks if the given element is present in this set.
        ///
        /// Returns `true` if the element is present, `false` otherwise.
        ///
        /// # Throws
        ///
        /// - `ClassCastException` - if the element type isn't compatible with the set
        /// - `NullPointerException` - if the given element is null and the set does not allow null values
        fn contains(element: JObject) -> bool,

        /// Returns the number of elements in this collection.
        ///
        /// Returns [i32::MAX] if the collection size is too large to be represented as an i32.
        fn size() -> jint,

        /// Returns `true` if this collection contains no elements.
        fn is_empty() -> bool,

        /// Returns an iterator (`java.util.Iterator`) over the elements in this collection.
        fn iterator() -> JIterator,

        /// Returns an array containing all of the elements in this collection.
        fn to_array() -> JObject[],
    }
}
