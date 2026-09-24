/// Trait for history handling.
pub trait History<T> {
    /// This is called with the current position that should
    /// be read from history. The `pos` represents the number
    /// of times the `Up`/`Down` arrow key has been pressed.
    /// This would normally be used as an index to some sort
    /// of vector.  If the `pos` does not have an entry, [`None`](Option::None)
    /// should be returned.
    fn read(&self, pos: usize) -> Option<String>;

    /// This is called with the next value you should store
    /// in history at the first location. Normally history
    /// is implemented as a FIFO queue.
    fn write(&mut self, val: &T);
}
