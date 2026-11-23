use crate::{
    Env,
    errors::*,
    objects::{JIterator, JObject, Reference},
    sys::jint,
};

crate::bind_java_type! {
    pub JList => "java.util.List",
    is_instance_of {
        collection = JCollection,
    },
    methods = {
        /// Returns the list element at the given `idx`
        ///
        /// # Throws
        ///
        /// - `IndexOutOfBoundsException` - if the index is out of range (index < 0 || index >= size())
        fn get(index: jint) -> JObject,
        /// Insert an element at a specific index
        fn insert {
            name = "add",
            sig = (index: jint, element: JObject) -> void,
        },
        /// Remove an element from the list by index
        ///
        /// Returns the removed element
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the remove operation is not supported
        /// - `IndexOutOfBoundsException` - if the index is out of bounds
        fn remove(index: jint) -> JObject,
    }
}

impl<'local> JList<'local> {
    /// Cast a local reference to a `JList`
    ///
    /// See [`JList::cast_local`] for more information.
    #[deprecated(
        since = "0.22.0",
        note = "use JList::cast_local instead or Env::new_cast_local_ref/cast_local/as_cast_local or Env::new_cast_global_ref/cast_global/as_cast_global"
    )]
    pub fn from_env<'any_local>(
        env: &mut Env<'_>,
        obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>,
    ) -> Result<JList<'any_local>> {
        env.cast_local::<JList>(obj)
    }

    /// Append an element to the list
    pub fn add(&self, env: &Env, value: &JObject) -> Result<bool> {
        self.as_collection().add(env, value)
    }

    /// Removes the first occurrence of `value` from this [JList], if it's present.
    ///
    /// Returns `true` if an element was removed.
    ///
    /// # Throws
    ///
    /// - `UnsupportedOperationException` - if the remove operation is not supported
    /// - `ClassCastException` - if the element type isn't compatible with the set
    /// - `NullPointerException` - if the given element is null and the set does not allow null values
    pub fn remove_item(&self, env: &Env<'_>, value: &JObject) -> Result<bool> {
        self.as_collection().remove(env, value)
    }

    /// Removes all of the elements from this list.
    ///
    /// # Throws
    ///
    /// - `UnsupportedOperationException` - if the clear operation is not supported
    pub fn clear(&self, env: &Env<'_>) -> Result<()> {
        self.as_collection().clear(env)
    }

    /// Get the size of the list
    pub fn size(&self, env: &Env) -> Result<jint> {
        self.as_collection().size(env)
    }

    /// Returns `true` if this list is empty.
    pub fn is_empty(&self, env: &Env<'_>) -> Result<bool> {
        self.as_collection().is_empty(env)
    }

    /// Pop the last element from the list
    ///
    /// # Deprecated
    ///
    /// Note that this is a non-standard utility API that first calls `size()`
    /// to determine the last index and so it's inherently race condition-prone
    /// and it's not recommended to use.
    #[deprecated(
        since = "0.22.0",
        note = "java.util.List has no pop() method. This non-standard utility will be removed from a future version"
    )]
    pub fn pop<'other_local_2>(
        &self,
        env: &mut Env<'other_local_2>,
    ) -> Result<Option<JObject<'other_local_2>>> {
        let size = self.size(env)?;
        if size == 0 {
            return Ok(None);
        }
        self.remove(env, size - 1).map(Some)
    }

    /// Returns an iterator (`java.util.Iterator`) over the elements in this
    /// list.
    ///
    /// The returned iterator does not implement [`std::iter::Iterator`] and
    /// cannot be used with a `for` loop. This is because its `next` method uses
    /// a `&mut Env` to call the Java iterator. Use a `while let` loop instead:
    ///
    /// ```rust,no_run
    /// # use jni::{errors::Result, Env, objects::{JList, JObject}};
    /// #
    /// # fn example(env: &mut Env, list: JList) -> Result<()> {
    /// use jni::objects::IntoAuto as _; // for .auto()
    /// let mut iterator = list.iter(env)?;
    ///
    /// while let Some(obj) = iterator.next(env)? {
    ///     let obj = obj.auto(); // Wrap as Auto<T> to avoid leaking while iterating
    ///
    ///     // Do something with `obj` here.
    /// }
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Each call to `next` creates a new local reference. To prevent excessive
    /// memory usage or overflow errors, the local reference should be deleted
    /// using [`Env::delete_local_ref`] or wrapped with
    /// [`crate::objects::IntoAuto::auto`] before the next loop iteration.
    /// Alternatively, if the list is known to have a small, predictable size,
    /// the loop could be wrapped in [`Env::with_local_frame`] to delete all of
    /// the local references at once.
    pub fn iter<'env_local>(&self, env: &mut Env<'env_local>) -> Result<JIterator<'env_local>> {
        self.as_collection().iterator(env)
    }
}
