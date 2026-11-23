use crate::{
    env::Env,
    errors::{Error, Result},
    objects::JObject,
};

crate::bind_java_type! {
    pub JIterator => "java.util.Iterator",
    methods {
        /// Returns true if the iteration has more elements.
        fn has_next() -> bool,

        priv fn _next() -> JObject,

        /// Removes the current element from the iteration (if supported by the iterator)
        ///
        /// This can only be called once after [Self::next] is called.
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` if the operation is not supported.
        /// - `IllegalStateException` if the iterator is in an invalid state (i.e [Self::next] has not been called).
        fn remove()
    }
}

impl<'local> JIterator<'local> {
    /// Returns the next element in the iteration, if it exists.
    ///
    /// Returns `Some(element)` if the iteration has more elements, or `None` if
    /// it has reached the end.
    ///
    /// This is like [`std::iter::Iterator::next`], but requires a parameter of
    /// type `&mut Env` in order to call into Java.
    ///
    /// Any exceptions thrown are assumed to be a `NoSuchElementException` and
    /// are caught + cleared before returning `None`.
    ///
    /// ## Beware of creating excessive local references in the current JNI stack frame
    ///
    /// This method creates a new local reference. To prevent excessive memory
    /// usage or overflow errors (when called repeatedly in a loop), the local
    /// reference should be deleted using [`Env::delete_local_ref`] or wrapped
    /// with [`crate::objects::IntoAuto::auto`] before the next loop iteration.
    /// Alternatively, if the collection is known to have a small, predictable
    /// size, the loop could be wrapped in [`Env::with_local_frame`] to delete
    /// all of the local references at once.
    pub fn next<'env_local>(
        &self,
        env: &mut Env<'env_local>,
    ) -> Result<Option<JObject<'env_local>>> {
        match self._next(env) {
            Ok(v) => Ok(Some(v)),
            Err(Error::JavaException) => {
                // Assume `NoSuchElementException` is thrown
                env.exception_clear();
                Ok(None)
            }
            Err(e) => Err(e),
        }
    }
}
