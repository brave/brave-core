//! Iterator utilities.
/// An iterator over chunks of input, producing `Vec<Item>` with a size of `size`, with the last chunk being the remainder and thus
/// potentially smaller than `size`.
pub struct Chunks<I> {
    /// The inner iterator to ask for items.
    pub inner: I,
    /// The size of chunks to produce
    pub size: usize,
}

impl<I, Item> Iterator for Chunks<I>
where
    I: Iterator<Item = Item>,
{
    type Item = Vec<Item>;

    fn next(&mut self) -> Option<Self::Item> {
        let mut res = Vec::with_capacity(self.size);
        let mut items_left = self.size;
        for item in &mut self.inner {
            res.push(item);
            items_left -= 1;
            if items_left == 0 {
                break;
            }
        }
        (!res.is_empty()).then_some(res)
    }
}
