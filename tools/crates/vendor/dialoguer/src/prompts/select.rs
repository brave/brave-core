use std::{io, ops::Rem};

use crate::paging::Paging;
use crate::theme::{SimpleTheme, TermThemeRenderer, Theme};

use console::{Key, Term};

/// Renders a select prompt.
///
/// User can select from one or more options.
/// Interaction returns index of an item selected in the order they appear in `item` invocation or `items` slice.
///
/// ## Examples
///
/// ```rust,no_run
/// use dialoguer::{console::Term, theme::ColorfulTheme, Select};
///
/// fn main() -> std::io::Result<()> {
///     let items = vec!["Item 1", "item 2"];
///     let selection = Select::with_theme(&ColorfulTheme::default())
///         .items(&items)
///         .default(0)
///         .interact_on_opt(&Term::stderr())?;
///
///     match selection {
///         Some(index) => println!("User selected item : {}", items[index]),
///         None => println!("User did not select anything")
///     }
///
///     Ok(())
/// }
/// ```
pub struct Select<'a> {
    default: usize,
    items: Vec<String>,
    prompt: Option<String>,
    report: bool,
    clear: bool,
    theme: &'a dyn Theme,
    max_length: Option<usize>,
}

impl Default for Select<'static> {
    fn default() -> Self {
        Self::new()
    }
}

impl Select<'static> {
    /// Creates a select prompt builder with default theme.
    pub fn new() -> Self {
        Self::with_theme(&SimpleTheme)
    }
}

impl Select<'_> {
    /// Indicates whether select menu should be erased from the screen after interaction.
    ///
    /// The default is to clear the menu.
    pub fn clear(&mut self, val: bool) -> &mut Self {
        self.clear = val;
        self
    }

    /// Sets initial selected element when select menu is rendered
    ///
    /// Element is indicated by the index at which it appears in `item` method invocation or `items` slice.
    pub fn default(&mut self, val: usize) -> &mut Self {
        self.default = val;
        self
    }

    /// Sets an optional max length for a page.
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
    ///
    /// ## Examples
    /// ```rust,no_run
    /// use dialoguer::Select;
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selection: usize = Select::new()
    ///         .item("Item 1")
    ///         .item("Item 2")
    ///         .interact()?;
    ///
    ///     Ok(())
    /// }
    /// ```
    pub fn item<T: ToString>(&mut self, item: T) -> &mut Self {
        self.items.push(item.to_string());
        self
    }

    /// Adds multiple items to the selector.
    ///
    /// ## Examples
    /// ```rust,no_run
    /// use dialoguer::Select;
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let items = vec!["Item 1", "Item 2"];
    ///     let selection: usize = Select::new()
    ///         .items(&items)
    ///         .interact()?;
    ///
    ///     println!("{}", items[selection]);
    ///
    ///     Ok(())
    /// }
    /// ```
    pub fn items<T: ToString>(&mut self, items: &[T]) -> &mut Self {
        for item in items {
            self.items.push(item.to_string());
        }
        self
    }

    /// Sets the select prompt.
    ///
    /// By default, when a prompt is set the system also prints out a confirmation after
    /// the selection. You can opt-out of this with [`report`](#method.report).
    ///
    /// ## Examples
    /// ```rust,no_run
    /// use dialoguer::Select;
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selection = Select::new()
    ///         .with_prompt("Which option do you prefer?")
    ///         .item("Option A")
    ///         .item("Option B")
    ///         .interact()?;
    ///
    ///     Ok(())
    /// }
    /// ```
    pub fn with_prompt<S: Into<String>>(&mut self, prompt: S) -> &mut Self {
        self.prompt = Some(prompt.into());
        self.report = true;
        self
    }

    /// Indicates whether to report the selected value after interaction.
    ///
    /// The default is to report the selection.
    pub fn report(&mut self, val: bool) -> &mut Self {
        self.report = val;
        self
    }

    /// Enables user interaction and returns the result.
    ///
    /// The user can select the items with the 'Space' bar or 'Enter' and the index of selected item will be returned.
    /// The dialog is rendered on stderr.
    /// Result contains `index` if user selected one of items using 'Enter'.
    /// This unlike [`interact_opt`](Self::interact_opt) does not allow to quit with 'Esc' or 'q'.
    #[inline]
    pub fn interact(&self) -> io::Result<usize> {
        self.interact_on(&Term::stderr())
    }

    /// Enables user interaction and returns the result.
    ///
    /// The user can select the items with the 'Space' bar or 'Enter' and the index of selected item will be returned.
    /// The dialog is rendered on stderr.
    /// Result contains `Some(index)` if user selected one of items using 'Enter' or `None` if user cancelled with 'Esc' or 'q'.
    #[inline]
    pub fn interact_opt(&self) -> io::Result<Option<usize>> {
        self.interact_on_opt(&Term::stderr())
    }

    /// Like [interact](#method.interact) but allows a specific terminal to be set.
    ///
    /// ## Examples
    ///```rust,no_run
    /// use dialoguer::{console::Term, Select};
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selection = Select::new()
    ///         .item("Option A")
    ///         .item("Option B")
    ///         .interact_on(&Term::stderr())?;
    ///
    ///     println!("User selected option at index {}", selection);
    ///
    ///     Ok(())
    /// }
    ///```
    #[inline]
    pub fn interact_on(&self, term: &Term) -> io::Result<usize> {
        self._interact_on(term, false)?
            .ok_or_else(|| io::Error::new(io::ErrorKind::Other, "Quit not allowed in this case"))
    }

    /// Like [`interact_opt`](Self::interact_opt) but allows a specific terminal to be set.
    ///
    /// ## Examples
    /// ```rust,no_run
    /// use dialoguer::{console::Term, Select};
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selection = Select::new()
    ///         .item("Option A")
    ///         .item("Option B")
    ///         .interact_on_opt(&Term::stdout())?;
    ///
    ///     match selection {
    ///         Some(position) => println!("User selected option at index {}", position),
    ///         None => println!("User did not select anything or exited using Esc or q")
    ///     }
    ///
    ///     Ok(())
    /// }
    /// ```
    #[inline]
    pub fn interact_on_opt(&self, term: &Term) -> io::Result<Option<usize>> {
        self._interact_on(term, true)
    }

    /// Like `interact` but allows a specific terminal to be set.
    fn _interact_on(&self, term: &Term, allow_quit: bool) -> io::Result<Option<usize>> {
        if self.items.is_empty() {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                "Empty list of items given to `Select`",
            ));
        }

        let mut paging = Paging::new(term, self.items.len(), self.max_length);
        let mut render = TermThemeRenderer::new(term, self.theme);
        let mut sel = self.default;

        let mut size_vec = Vec::new();

        for items in self
            .items
            .iter()
            .flat_map(|i| i.split('\n'))
            .collect::<Vec<_>>()
        {
            let size = &items.len();
            size_vec.push(*size);
        }

        term.hide_cursor()?;

        loop {
            if let Some(ref prompt) = self.prompt {
                paging.render_prompt(|paging_info| render.select_prompt(prompt, paging_info))?;
            }

            for (idx, item) in self
                .items
                .iter()
                .enumerate()
                .skip(paging.current_page * paging.capacity)
                .take(paging.capacity)
            {
                render.select_prompt_item(item, sel == idx)?;
            }

            term.flush()?;

            match term.read_key()? {
                Key::ArrowDown | Key::Tab | Key::Char('j') => {
                    if sel == !0 {
                        sel = 0;
                    } else {
                        sel = (sel as u64 + 1).rem(self.items.len() as u64) as usize;
                    }
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
                Key::ArrowUp | Key::BackTab | Key::Char('k') => {
                    if sel == !0 {
                        sel = self.items.len() - 1;
                    } else {
                        sel = ((sel as i64 - 1 + self.items.len() as i64)
                            % (self.items.len() as i64)) as usize;
                    }
                }
                Key::ArrowLeft | Key::Char('h') => {
                    if paging.active {
                        sel = paging.previous_page();
                    }
                }
                Key::ArrowRight | Key::Char('l') => {
                    if paging.active {
                        sel = paging.next_page();
                    }
                }

                Key::Enter | Key::Char(' ') if sel != !0 => {
                    if self.clear {
                        render.clear()?;
                    }

                    if let Some(ref prompt) = self.prompt {
                        if self.report {
                            render.select_prompt_selection(prompt, &self.items[sel])?;
                        }
                    }

                    term.show_cursor()?;
                    term.flush()?;

                    return Ok(Some(sel));
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

impl<'a> Select<'a> {
    /// Creates a select prompt builder with a specific theme.
    ///
    /// ## Examples
    /// ```rust,no_run
    /// use dialoguer::{
    ///     Select,
    ///     theme::ColorfulTheme
    /// };
    ///
    /// fn main() -> std::io::Result<()> {
    ///     let selection = Select::with_theme(&ColorfulTheme::default())
    ///         .item("Option A")
    ///         .item("Option B")
    ///         .interact()?;
    ///
    ///     Ok(())
    /// }
    /// ```
    pub fn with_theme(theme: &'a dyn Theme) -> Self {
        Self {
            default: !0,
            items: vec![],
            prompt: None,
            report: false,
            clear: true,
            max_length: None,
            theme,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_str() {
        let selections = &[
            "Ice Cream",
            "Vanilla Cupcake",
            "Chocolate Muffin",
            "A Pile of sweet, sweet mustard",
        ];

        assert_eq!(
            Select::new().default(0).items(&selections[..]).items,
            selections
        );
    }

    #[test]
    fn test_string() {
        let selections = vec!["a".to_string(), "b".to_string()];

        assert_eq!(
            Select::new().default(0).items(&selections[..]).items,
            selections
        );
    }

    #[test]
    fn test_ref_str() {
        let a = "a";
        let b = "b";

        let selections = &[a, b];

        assert_eq!(
            Select::new().default(0).items(&selections[..]).items,
            selections
        );
    }
}
