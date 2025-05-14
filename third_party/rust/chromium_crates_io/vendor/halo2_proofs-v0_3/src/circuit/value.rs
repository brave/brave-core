use std::borrow::Borrow;
use std::ops::{Add, Mul, Neg, Sub};

use group::ff::Field;

use crate::plonk::{Assigned, Error};

/// A value that might exist within a circuit.
///
/// This behaves like `Option<V>` but differs in two key ways:
/// - It does not expose the enum cases, or provide an `Option::unwrap` equivalent. This
///   helps to ensure that unwitnessed values correctly propagate.
/// - It provides pass-through implementations of common traits such as `Add` and `Mul`,
///   for improved usability.
#[derive(Clone, Copy, Debug)]
pub struct Value<V> {
    inner: Option<V>,
}

impl<V> Default for Value<V> {
    fn default() -> Self {
        Self::unknown()
    }
}

impl<V> Value<V> {
    /// Constructs an unwitnessed value.
    pub const fn unknown() -> Self {
        Self { inner: None }
    }

    /// Constructs a known value.
    ///
    /// # Examples
    ///
    /// ```
    /// use halo2_proofs::circuit::Value;
    ///
    /// let v = Value::known(37);
    /// ```
    pub const fn known(value: V) -> Self {
        Self { inner: Some(value) }
    }

    /// Obtains the inner value for assigning into the circuit.
    ///
    /// Returns `Error::Synthesis` if this is [`Value::unknown()`].
    pub(crate) fn assign(self) -> Result<V, Error> {
        self.inner.ok_or(Error::Synthesis)
    }

    /// Converts from `&Value<V>` to `Value<&V>`.
    pub fn as_ref(&self) -> Value<&V> {
        Value {
            inner: self.inner.as_ref(),
        }
    }

    /// Converts from `&mut Value<V>` to `Value<&mut V>`.
    pub fn as_mut(&mut self) -> Value<&mut V> {
        Value {
            inner: self.inner.as_mut(),
        }
    }

    /// ONLY FOR INTERNAL CRATE USAGE; DO NOT EXPOSE!
    pub(crate) fn into_option(self) -> Option<V> {
        self.inner
    }

    /// Enforces an assertion on the contained value, if known.
    ///
    /// The assertion is ignored if `self` is [`Value::unknown()`]. Do not try to enforce
    /// circuit constraints with this method!
    ///
    /// # Panics
    ///
    /// Panics if `f` returns `false`.
    pub fn assert_if_known<F: FnOnce(&V) -> bool>(&self, f: F) {
        if let Some(value) = self.inner.as_ref() {
            assert!(f(value));
        }
    }

    /// Checks the contained value for an error condition, if known.
    ///
    /// The error check is ignored if `self` is [`Value::unknown()`]. Do not try to
    /// enforce circuit constraints with this method!
    pub fn error_if_known_and<F: FnOnce(&V) -> bool>(&self, f: F) -> Result<(), Error> {
        match self.inner.as_ref() {
            Some(value) if f(value) => Err(Error::Synthesis),
            _ => Ok(()),
        }
    }

    /// Maps a `Value<V>` to `Value<W>` by applying a function to the contained value.
    pub fn map<W, F: FnOnce(V) -> W>(self, f: F) -> Value<W> {
        Value {
            inner: self.inner.map(f),
        }
    }

    /// Returns [`Value::unknown()`] if the value is [`Value::unknown()`], otherwise calls
    /// `f` with the wrapped value and returns the result.
    pub fn and_then<W, F: FnOnce(V) -> Value<W>>(self, f: F) -> Value<W> {
        match self.inner {
            Some(v) => f(v),
            None => Value::unknown(),
        }
    }

    /// Zips `self` with another `Value`.
    ///
    /// If `self` is `Value::known(s)` and `other` is `Value::known(o)`, this method
    /// returns `Value::known((s, o))`. Otherwise, [`Value::unknown()`] is returned.
    pub fn zip<W>(self, other: Value<W>) -> Value<(V, W)> {
        Value {
            inner: self.inner.zip(other.inner),
        }
    }
}

impl<V, W> Value<(V, W)> {
    /// Unzips a value containing a tuple of two values.
    ///
    /// If `self` is `Value::known((a, b)), this method returns
    /// `(Value::known(a), Value::known(b))`. Otherwise,
    /// `(Value::unknown(), Value::unknown())` is returned.
    pub fn unzip(self) -> (Value<V>, Value<W>) {
        match self.inner {
            Some((a, b)) => (Value::known(a), Value::known(b)),
            None => (Value::unknown(), Value::unknown()),
        }
    }
}

impl<V> Value<&V> {
    /// Maps a `Value<&V>` to a `Value<V>` by copying the contents of the value.
    #[must_use = "`self` will be dropped if the result is not used"]
    pub fn copied(self) -> Value<V>
    where
        V: Copy,
    {
        Value {
            inner: self.inner.copied(),
        }
    }

    /// Maps a `Value<&V>` to a `Value<V>` by cloning the contents of the value.
    #[must_use = "`self` will be dropped if the result is not used"]
    pub fn cloned(self) -> Value<V>
    where
        V: Clone,
    {
        Value {
            inner: self.inner.cloned(),
        }
    }
}

impl<V> Value<&mut V> {
    /// Maps a `Value<&mut V>` to a `Value<V>` by copying the contents of the value.
    #[must_use = "`self` will be dropped if the result is not used"]
    pub fn copied(self) -> Value<V>
    where
        V: Copy,
    {
        Value {
            inner: self.inner.copied(),
        }
    }

    /// Maps a `Value<&mut V>` to a `Value<V>` by cloning the contents of the value.
    #[must_use = "`self` will be dropped if the result is not used"]
    pub fn cloned(self) -> Value<V>
    where
        V: Clone,
    {
        Value {
            inner: self.inner.cloned(),
        }
    }
}

impl<V: Copy, const LEN: usize> Value<[V; LEN]> {
    /// Transposes a `Value<[V; LEN]>` into a `[Value<V>; LEN]`.
    ///
    /// [`Value::unknown()`] will be mapped to `[Value::unknown(); LEN]`.
    pub fn transpose_array(self) -> [Value<V>; LEN] {
        let mut ret = [Value::unknown(); LEN];
        if let Some(arr) = self.inner {
            for (entry, value) in ret.iter_mut().zip(arr) {
                *entry = Value::known(value);
            }
        }
        ret
    }
}

impl<V, I> Value<I>
where
    I: IntoIterator<Item = V>,
    I::IntoIter: ExactSizeIterator,
{
    /// Transposes a `Value<impl IntoIterator<Item = V>>` into a `Vec<Value<V>>`.
    ///
    /// [`Value::unknown()`] will be mapped to `vec![Value::unknown(); length]`.
    ///
    /// # Panics
    ///
    /// Panics if `self` is `Value::known(values)` and `values.len() != length`.
    pub fn transpose_vec(self, length: usize) -> Vec<Value<V>> {
        match self.inner {
            Some(values) => {
                let values = values.into_iter();
                assert_eq!(values.len(), length);
                values.map(Value::known).collect()
            }
            None => (0..length).map(|_| Value::unknown()).collect(),
        }
    }
}

//
// FromIterator
//

impl<A, V: FromIterator<A>> FromIterator<Value<A>> for Value<V> {
    /// Takes each element in the [`Iterator`]: if it is [`Value::unknown()`], no further
    /// elements are taken, and the [`Value::unknown()`] is returned. Should no
    /// [`Value::unknown()`] occur, a container of type `V` containing the values of each
    /// [`Value`] is returned.
    fn from_iter<I: IntoIterator<Item = Value<A>>>(iter: I) -> Self {
        Self {
            inner: iter.into_iter().map(|v| v.inner).collect(),
        }
    }
}

//
// Neg
//

impl<V: Neg> Neg for Value<V> {
    type Output = Value<V::Output>;

    fn neg(self) -> Self::Output {
        Value {
            inner: self.inner.map(|v| -v),
        }
    }
}

//
// Add
//

impl<V, O> Add for Value<V>
where
    V: Add<Output = O>,
{
    type Output = Value<O>;

    fn add(self, rhs: Self) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a + b),
        }
    }
}

impl<V, O> Add for &Value<V>
where
    for<'v> &'v V: Add<Output = O>,
{
    type Output = Value<O>;

    fn add(self, rhs: Self) -> Self::Output {
        Value {
            inner: self
                .inner
                .as_ref()
                .zip(rhs.inner.as_ref())
                .map(|(a, b)| a + b),
        }
    }
}

impl<V, O> Add<Value<&V>> for Value<V>
where
    for<'v> V: Add<&'v V, Output = O>,
{
    type Output = Value<O>;

    fn add(self, rhs: Value<&V>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a + b),
        }
    }
}

impl<V, O> Add<Value<V>> for Value<&V>
where
    for<'v> &'v V: Add<V, Output = O>,
{
    type Output = Value<O>;

    fn add(self, rhs: Value<V>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a + b),
        }
    }
}

impl<V, O> Add<&Value<V>> for Value<V>
where
    for<'v> V: Add<&'v V, Output = O>,
{
    type Output = Value<O>;

    fn add(self, rhs: &Self) -> Self::Output {
        self + rhs.as_ref()
    }
}

impl<V, O> Add<Value<V>> for &Value<V>
where
    for<'v> &'v V: Add<V, Output = O>,
{
    type Output = Value<O>;

    fn add(self, rhs: Value<V>) -> Self::Output {
        self.as_ref() + rhs
    }
}

//
// Sub
//

impl<V, O> Sub for Value<V>
where
    V: Sub<Output = O>,
{
    type Output = Value<O>;

    fn sub(self, rhs: Self) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a - b),
        }
    }
}

impl<V, O> Sub for &Value<V>
where
    for<'v> &'v V: Sub<Output = O>,
{
    type Output = Value<O>;

    fn sub(self, rhs: Self) -> Self::Output {
        Value {
            inner: self
                .inner
                .as_ref()
                .zip(rhs.inner.as_ref())
                .map(|(a, b)| a - b),
        }
    }
}

impl<V, O> Sub<Value<&V>> for Value<V>
where
    for<'v> V: Sub<&'v V, Output = O>,
{
    type Output = Value<O>;

    fn sub(self, rhs: Value<&V>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a - b),
        }
    }
}

impl<V, O> Sub<Value<V>> for Value<&V>
where
    for<'v> &'v V: Sub<V, Output = O>,
{
    type Output = Value<O>;

    fn sub(self, rhs: Value<V>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a - b),
        }
    }
}

impl<V, O> Sub<&Value<V>> for Value<V>
where
    for<'v> V: Sub<&'v V, Output = O>,
{
    type Output = Value<O>;

    fn sub(self, rhs: &Self) -> Self::Output {
        self - rhs.as_ref()
    }
}

impl<V, O> Sub<Value<V>> for &Value<V>
where
    for<'v> &'v V: Sub<V, Output = O>,
{
    type Output = Value<O>;

    fn sub(self, rhs: Value<V>) -> Self::Output {
        self.as_ref() - rhs
    }
}

//
// Mul
//

impl<V, O> Mul for Value<V>
where
    V: Mul<Output = O>,
{
    type Output = Value<O>;

    fn mul(self, rhs: Self) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a * b),
        }
    }
}

impl<V, O> Mul for &Value<V>
where
    for<'v> &'v V: Mul<Output = O>,
{
    type Output = Value<O>;

    fn mul(self, rhs: Self) -> Self::Output {
        Value {
            inner: self
                .inner
                .as_ref()
                .zip(rhs.inner.as_ref())
                .map(|(a, b)| a * b),
        }
    }
}

impl<V, O> Mul<Value<&V>> for Value<V>
where
    for<'v> V: Mul<&'v V, Output = O>,
{
    type Output = Value<O>;

    fn mul(self, rhs: Value<&V>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a * b),
        }
    }
}

impl<V, O> Mul<Value<V>> for Value<&V>
where
    for<'v> &'v V: Mul<V, Output = O>,
{
    type Output = Value<O>;

    fn mul(self, rhs: Value<V>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a * b),
        }
    }
}

impl<V, O> Mul<&Value<V>> for Value<V>
where
    for<'v> V: Mul<&'v V, Output = O>,
{
    type Output = Value<O>;

    fn mul(self, rhs: &Self) -> Self::Output {
        self * rhs.as_ref()
    }
}

impl<V, O> Mul<Value<V>> for &Value<V>
where
    for<'v> &'v V: Mul<V, Output = O>,
{
    type Output = Value<O>;

    fn mul(self, rhs: Value<V>) -> Self::Output {
        self.as_ref() * rhs
    }
}

//
// Assigned<Field>
//

impl<F: Field> From<Value<F>> for Value<Assigned<F>> {
    fn from(value: Value<F>) -> Self {
        Self {
            inner: value.inner.map(Assigned::from),
        }
    }
}

impl<F: Field> Add<Value<F>> for Value<Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn add(self, rhs: Value<F>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a + b),
        }
    }
}

impl<F: Field> Add<F> for Value<Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn add(self, rhs: F) -> Self::Output {
        self + Value::known(rhs)
    }
}

impl<F: Field> Add<Value<F>> for Value<&Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn add(self, rhs: Value<F>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a + b),
        }
    }
}

impl<F: Field> Add<F> for Value<&Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn add(self, rhs: F) -> Self::Output {
        self + Value::known(rhs)
    }
}

impl<F: Field> Sub<Value<F>> for Value<Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn sub(self, rhs: Value<F>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a - b),
        }
    }
}

impl<F: Field> Sub<F> for Value<Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn sub(self, rhs: F) -> Self::Output {
        self - Value::known(rhs)
    }
}

impl<F: Field> Sub<Value<F>> for Value<&Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn sub(self, rhs: Value<F>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a - b),
        }
    }
}

impl<F: Field> Sub<F> for Value<&Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn sub(self, rhs: F) -> Self::Output {
        self - Value::known(rhs)
    }
}

impl<F: Field> Mul<Value<F>> for Value<Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn mul(self, rhs: Value<F>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a * b),
        }
    }
}

impl<F: Field> Mul<F> for Value<Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn mul(self, rhs: F) -> Self::Output {
        self * Value::known(rhs)
    }
}

impl<F: Field> Mul<Value<F>> for Value<&Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn mul(self, rhs: Value<F>) -> Self::Output {
        Value {
            inner: self.inner.zip(rhs.inner).map(|(a, b)| a * b),
        }
    }
}

impl<F: Field> Mul<F> for Value<&Assigned<F>> {
    type Output = Value<Assigned<F>>;

    fn mul(self, rhs: F) -> Self::Output {
        self * Value::known(rhs)
    }
}

impl<V> Value<V> {
    /// Returns the field element corresponding to this value.
    pub fn to_field<F: Field>(&self) -> Value<Assigned<F>>
    where
        for<'v> Assigned<F>: From<&'v V>,
    {
        Value {
            inner: self.inner.as_ref().map(|v| v.into()),
        }
    }

    /// Returns the field element corresponding to this value.
    pub fn into_field<F: Field>(self) -> Value<Assigned<F>>
    where
        V: Into<Assigned<F>>,
    {
        Value {
            inner: self.inner.map(|v| v.into()),
        }
    }

    /// Doubles this field element.
    ///
    /// # Examples
    ///
    /// If you have a `Value<F: Field>`, convert it to `Value<Assigned<F>>` first:
    /// ```
    /// # use pasta_curves::pallas::Base as F;
    /// use halo2_proofs::{circuit::Value, plonk::Assigned};
    ///
    /// let v = Value::known(F::from(2));
    /// let v: Value<Assigned<F>> = v.into();
    /// v.double();
    /// ```
    pub fn double<F: Field>(&self) -> Value<Assigned<F>>
    where
        V: Borrow<Assigned<F>>,
    {
        Value {
            inner: self.inner.as_ref().map(|v| v.borrow().double()),
        }
    }

    /// Squares this field element.
    pub fn square<F: Field>(&self) -> Value<Assigned<F>>
    where
        V: Borrow<Assigned<F>>,
    {
        Value {
            inner: self.inner.as_ref().map(|v| v.borrow().square()),
        }
    }

    /// Cubes this field element.
    pub fn cube<F: Field>(&self) -> Value<Assigned<F>>
    where
        V: Borrow<Assigned<F>>,
    {
        Value {
            inner: self.inner.as_ref().map(|v| v.borrow().cube()),
        }
    }

    /// Inverts this assigned value (taking the inverse of zero to be zero).
    pub fn invert<F: Field>(&self) -> Value<Assigned<F>>
    where
        V: Borrow<Assigned<F>>,
    {
        Value {
            inner: self.inner.as_ref().map(|v| v.borrow().invert()),
        }
    }
}

impl<F: Field> Value<Assigned<F>> {
    /// Evaluates this value directly, performing an unbatched inversion if necessary.
    ///
    /// If the denominator is zero, the returned value is zero.
    pub fn evaluate(self) -> Value<F> {
        Value {
            inner: self.inner.map(|v| v.evaluate()),
        }
    }
}
