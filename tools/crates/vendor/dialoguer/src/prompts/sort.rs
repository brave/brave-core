use std::{io, ops::Rem};

use crate::{
    theme::{SimpleTheme, TermThemeRenderer, Theme},
    Paging,
};

use console::{Key, Term};

/// Renders a sort prompt.
///
/// Returns list of indices in original items list sorted according to user input.
///
/// ## Example usage
/// ```rust,no_run
/// use dialoguer::Sort;
///
/// # fn test() -> Result<(), Box<dyn std::error::Error>> {
/// let items_to_order = vec!["Item 1", "Item 2", "Item 3"];
/// let ordered = Sort::new()
///     .with_prompt("Order the items")
///     .items(&items_to_order)
///     .interact()?;
/// # Ok(())
/// # }
/// ```
pub struct Sort<'a> {
    items: Vec<String>,
    prompt: Option<String>,
    report: bool,
    clear: bool,
    max_length: Option<usize>,
    theme: &'a dyn Theme,
}

impl Default for Sort<'static> {
    fn default() -> Self {
        Self::new()
    }
}

impl Sort<'static> {
    /// Creates a sort prompt.
    pub fn new() -> Self {
        Self::with_theme(&SimpleTheme)
    }
}

impl Sort<'_> {
    /// Sets the clear behavior of the menu.
    ///
    /// The default is to clear the menu after user interaction.
    pub fn clear(&mut self, val: bool) -> &mut Self {
        self.clear = val;
        self
    }

    /// Sets an optional max length for a page
    ///
    /// Max length is disabled by None
    pub fn max_length(&mut self, val: usize) -> &mut Self {
        // Paging subtracts two from the capacity, paging does this to
        // make an offset for the page indicator. So to make sure that
        // we can show the intended amount of items we need to add two
        // to our value.
        self.max_length = Some(val + 2);
        self
    }

    /// Add a single item to the selector.
    pub fn item<T: ToString>(&mut self, item: T) -> &mut Self {
        self.items.push(item.to_string());
        self
    }

    /// Adds multiple items to the selector.
    pub fn items<T: ToString>(&mut self, items: &[T]) -> &mut Self {
        for item in items {
            self.items.push(item.to_string());
        }
        self
    }

    /// Prefaces the menu with a prompt.
    ///
    /// By default, when a prompt is set the system also prints out a confirmation after
    /// the selection. You can opt-out of this with [`report`](#method.report).
    pub fn with_prompt<S: Into<String>>(&mut self, prompt: S) -> &mut Self {
        self.prompt = Some(prompt.into());
        self
    }

    /// Indicates whether to report the selected order after interaction.
    ///
    /// The default is to report the selected order.
    pub fn report(&mut self, val: bool) -> &mut Self {
        self.report = val;
        self
    }

    /// Enables user interaction and returns the result.
    ///
    /// The user can order the items with the 'Space' bar and the arrows. On 'Enter' ordered list of the incides of items will be returned.
    /// The dialog is rendered on stderr.
    /// Result contains `Vec<index>` if user hit 'Enter'.
    /// This unlike [`interact_opt`](Self::interact_opt) does not allow to quit with 'Esc' or 'q'.
    #[inline]
    pub fn interact(&self) -> io::Result<Vec<usize>> {
        self.interact_on(&Term::stderr())
    }

    /// Enables user interaction and returns the result.
    ///
    /// The user can order the items with the 'Space' bar and the arrows. On 'Enter' ordered list of the incides of items will be returned.
    /// The dialog is rendered on stderr.
    /// Result contains `Some(Vec<index>)` if user hit 'Enter' or `None` if user cancelled with 'Esc' or 'q'.
    #[inline]
    pub fn interact_opt(&self) -> io::Result<Option<Vec<usize>>> {
        self.interact_on_opt(&Term::stderr())
    }

    /// Like [interact](#method.interact) but allows a specific terminal to be set.
    ///
    /// ## Examples
    ///```rust,no_run
    /// use dialoguer::Sort;
    /// use console::Term;
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selections = Sort::new()
    ///         .item("Option A")
    ///         .item("Option B")
    ///         .interact_on(&Term::stderr())?;
    ///
    ///     println!("User sorted options as indices {:?}", selections);
    ///
    ///     Ok(())
    /// }
    ///```
    #[inline]
    pub fn interact_on(&self, term: &Term) -> io::Result<Vec<usize>> {
        self._interact_on(term, false)?
            .ok_or_else(|| io::Error::new(io::ErrorKind::Other, "Quit not allowed in this case"))
    }

    /// Like [`interact_opt`](Self::interact_opt) but allows a specific terminal to be set.
    ///
    /// ## Examples
    /// ```rust,no_run
    /// use dialoguer::Sort;
    /// use console::Term;
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selections = Sort::new()
    ///         .item("Option A")
    ///         .item("Option B")
    ///         .interact_on_opt(&Term::stdout())?;
    ///
    ///     match selections {
    ///         Some(positions) => println!("User sorted options as indices {:?}", positions),
    ///         None => println!("User exited using Esc or q")
    ///     }
    ///
    ///     Ok(())
    /// }
    /// ```
    #[inline]
    pub fn interact_on_opt(&self, term: &Term) -> io::Result<Option<Vec<usize>>> {
        self._interact_on(term, true)
    }

    fn _interact_on(&self, term: &Term, allow_quit: bool) -> io::Result<Option<Vec<usize>>> {
        if self.items.is_empty() {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                "Empty list of items given to `Sort`",
            ));
        }

        let mut paging = Paging::new(term, self.items.len(), self.max_length);
        let mut render = TermThemeRenderer::new(term, self.theme);
        let mut sel = 0;

        let mut size_vec = Vec::new();

        for items in self.items.iter().as_slice() {
            let size = &items.len();
            size_vec.push(*size);
        }

        let mut order: Vec<_> = (0..self.items.len()).collect();
        let mut checked: bool = false;

        term.hide_cursor()?;

        loop {
            if let Some(ref prompt) = self.prompt {
                paging.render_prompt(|paging_info| render.sort_prompt(prompt, paging_info))?;
            }

            for (idx, item) in order
                .iter()
                .enumerate()
                .skip(paging.current_page * paging.capacity)
                .take(paging.capacity)
            {
                render.sort_prompt_item(&self.items[*item], checked, sel == idx)?;
            }

            term.flush()?;

            match term.read_key()? {
                Key::ArrowDown | Key::Tab | Key::Char('j') => {
                    let old_sel = sel;

                    if sel == !0 {
                        sel = 0;
                    } else {
                        sel = (sel as u64 + 1).rem(self.items.len() as u64) as usize;
                    }

                    if checked && old_sel != sel {
                        order.swap(old_sel, sel);
                    }
                }
                Key::ArrowUp | Key::BackTab | Key::Char('k') => {
                    let old_sel = sel;

                    if sel == !0 {
                        sel = self.items.len() - 1;
                    } else {
                        sel = ((sel as i64 - 1 + self.items.len() as i64)
                            % (self.items.len() as i64)) as usize;
                    }

                    if checked && old_sel != sel {
                        order.swap(old_sel, sel);
                    }
                }
                Key::ArrowLeft | Key::Char('h') => {
                    if paging.active {
                        let old_sel = sel;
                        let old_page = paging.current_page;

                        sel = paging.previous_page();

                        if checked {
                            let indexes: Vec<_> = if old_page == 0 {
                                let indexes1: Vec<_> = (0..=old_sel).rev().collect();
                                let indexes2: Vec<_> = (sel..self.items.len()).rev().collect();
                                [indexes1, indexes2].concat()
                            } else {
                                (sel..=old_sel).rev().collect()
                            };

                            for index in 0..(indexes.len() - 1) {
                                order.swap(indexes[index], indexes[index + 1]);
                            }
                        }
                    }
                }
                Key::ArrowRight | Key::Char('l') => {
                    if paging.active {
                        let old_sel = sel;
                        let old_page = paging.current_page;

                        sel = paging.next_page();

                        if checked {
                            let indexes: Vec<_> = if old_page == paging.pages - 1 {
                                let indexes1: Vec<_> = (old_sel..self.items.len()).collect();
                                let indexes2: Vec<_> = vec![0];
                                [indexes1, indexes2].concat()
                            } else {
                                (old_sel..=sel).collect()
                            };

                            for index in 0..(indexes.len() - 1) {
                                order.swap(indexes[index], indexes[index + 1]);
                            }
                        }
                    }
                }
                Key::Char(' ') => {
                    checked = !checked;
                }
                Key::Escape | Key::Char('q') => {
                    if allow_quit {
                        if self.clear {
                            render.clear()?;
                        } else {
                            term.clear_last_lines(paging.capacity)?;
                        }

                        term.show_cursor()?;
                        term.flush()?;

                        return Ok(None);
                    }
                }
                Key::Enter => {
                    if self.clear {
                        render.clear()?;
                    }

                    if let Some(ref prompt) = self.prompt {
                        if self.report {
                            let list: Vec<_> = order
                                .iter()
                                .enumerate()
                                .map(|(_, item)| self.items[*item].as_str())
                                .collect();
                            render.sort_prompt_selection(prompt, &list[..])?;
                        }
                    }

                    term.show_cursor()?;
                    term.flush()?;

                    return Ok(Some(order));
                }
                _ => {}
            }

            paging.update(sel)?;

            if paging.active {
                render.clear()?;
            } else {
                render.clear_preserve_prompt(&size_vec)?;
            }
        }
    }
}

impl<'a> Sort<'a> {
    /// Creates a sort prompt with a specific theme.
    pub fn with_theme(theme: &'a dyn Theme) -> Self {
        Self {
            items: vec![],
            clear: true,
            prompt: None,
            report: true,
            max_length: None,
            theme,
        }
    }
}
