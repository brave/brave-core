use crate::theme::{SimpleTheme, TermThemeRenderer, Theme};
use console::{Key, Term};
use fuzzy_matcher::FuzzyMatcher;
use std::{io, ops::Rem};

/// Renders a selection menu that user can fuzzy match to reduce set.
///
/// User can use fuzzy search to limit selectable items.
/// Interaction returns index of an item selected in the order they appear in `item` invocation or `items` slice.
///
/// ## Examples
///
/// ```rust,no_run
/// use dialoguer::{
///     FuzzySelect,
///     theme::ColorfulTheme
/// };
/// use console::Term;
///
/// fn main() -> std::io::Result<()> {
///     let items = vec!["Item 1", "item 2"];
///     let selection = FuzzySelect::with_theme(&ColorfulTheme::default())
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

pub struct FuzzySelect<'a> {
    default: Option<usize>,
    items: Vec<String>,
    prompt: String,
    report: bool,
    clear: bool,
    highlight_matches: bool,
    max_length: Option<usize>,
    theme: &'a dyn Theme,
    /// Search string that a fuzzy search with start with.
    /// Defaults to an empty string.
    initial_text: String,
}

impl Default for FuzzySelect<'static> {
    fn default() -> Self {
        Self::new()
    }
}

impl FuzzySelect<'static> {
    /// Creates the prompt with a specific text.
    pub fn new() -> Self {
        Self::with_theme(&SimpleTheme)
    }
}

impl FuzzySelect<'_> {
    /// Sets the clear behavior of the menu.
    ///
    /// The default is to clear the menu.
    pub fn clear(&mut self, val: bool) -> &mut Self {
        self.clear = val;
        self
    }

    /// Sets a default for the menu
    pub fn default(&mut self, val: usize) -> &mut Self {
        self.default = Some(val);
        self
    }

    /// Add a single item to the fuzzy selector.
    pub fn item<T: ToString>(&mut self, item: T) -> &mut Self {
        self.items.push(item.to_string());
        self
    }

    /// Adds multiple items to the fuzzy selector.
    pub fn items<T: ToString>(&mut self, items: &[T]) -> &mut Self {
        for item in items {
            self.items.push(item.to_string());
        }
        self
    }

    /// Sets the search text that a fuzzy search starts with.
    pub fn with_initial_text<S: Into<String>>(&mut self, initial_text: S) -> &mut Self {
        self.initial_text = initial_text.into();
        self
    }

    /// Prefaces the menu with a prompt.
    ///
    /// When a prompt is set the system also prints out a confirmation after
    /// the fuzzy selection.
    pub fn with_prompt<S: Into<String>>(&mut self, prompt: S) -> &mut Self {
        self.prompt = prompt.into();
        self
    }

    /// Indicates whether to report the selected value after interaction.
    ///
    /// The default is to report the selection.
    pub fn report(&mut self, val: bool) -> &mut Self {
        self.report = val;
        self
    }

    /// Indicates whether to highlight matched indices
    ///
    /// The default is to highlight the indices
    pub fn highlight_matches(&mut self, val: bool) -> &mut Self {
        self.highlight_matches = val;
        self
    }

    /// Sets the maximum number of visible options.
    ///
    /// The default is the height of the terminal minus 2.
    pub fn max_length(&mut self, rows: usize) -> &mut Self {
        self.max_length = Some(rows);
        self
    }

    /// Enables user interaction and returns the result.
    ///
    /// The user can select the items using 'Enter' and the index of selected item will be returned.
    /// The dialog is rendered on stderr.
    /// Result contains `index` of selected item if user hit 'Enter'.
    /// This unlike [interact_opt](#method.interact_opt) does not allow to quit with 'Esc' or 'q'.
    #[inline]
    pub fn interact(&self) -> io::Result<usize> {
        self.interact_on(&Term::stderr())
    }

    /// Enables user interaction and returns the result.
    ///
    /// The user can select the items using 'Enter' and the index of selected item will be returned.
    /// The dialog is rendered on stderr.
    /// Result contains `Some(index)` if user hit 'Enter' or `None` if user cancelled with 'Esc' or 'q'.
    #[inline]
    pub fn interact_opt(&self) -> io::Result<Option<usize>> {
        self.interact_on_opt(&Term::stderr())
    }

    /// Like `interact` but allows a specific terminal to be set.
    #[inline]
    pub fn interact_on(&self, term: &Term) -> io::Result<usize> {
        self._interact_on(term, false)?
            .ok_or_else(|| io::Error::new(io::ErrorKind::Other, "Quit not allowed in this case"))
    }

    /// Like `interact` but allows a specific terminal to be set.
    #[inline]
    pub fn interact_on_opt(&self, term: &Term) -> io::Result<Option<usize>> {
        self._interact_on(term, true)
    }

    /// Like `interact` but allows a specific terminal to be set.
    fn _interact_on(&self, term: &Term, allow_quit: bool) -> io::Result<Option<usize>> {
        // Place cursor at the end of the search term
        let mut position = self.initial_text.len();
        let mut search_term = self.initial_text.to_owned();

        let mut render = TermThemeRenderer::new(term, self.theme);
        let mut sel = self.default;

        let mut size_vec = Vec::new();
        for items in self.items.iter().as_slice() {
            let size = &items.len();
            size_vec.push(*size);
        }

        // Fuzzy matcher
        let matcher = fuzzy_matcher::skim::SkimMatcherV2::default();

        // Subtract -2 because we need space to render the prompt.
        let visible_term_rows = (term.size().0 as usize).max(3) - 2;
        let visible_term_rows = self
            .max_length
            .unwrap_or(visible_term_rows)
            .min(visible_term_rows);
        // Variable used to determine if we need to scroll through the list.
        let mut starting_row = 0;

        term.hide_cursor()?;

        loop {
            render.clear()?;
            render.fuzzy_select_prompt(self.prompt.as_str(), &search_term, position)?;

            // Maps all items to a tuple of item and its match score.
            let mut filtered_list = self
                .items
                .iter()
                .map(|item| (item, matcher.fuzzy_match(item, &search_term)))
                .filter_map(|(item, score)| score.map(|s| (item, s)))
                .collect::<Vec<_>>();

            // Renders all matching items, from best match to worst.
            filtered_list.sort_unstable_by(|(_, s1), (_, s2)| s2.cmp(s1));

            for (idx, (item, _)) in filtered_list
                .iter()
                .enumerate()
                .skip(starting_row)
                .take(visible_term_rows)
            {
                render.fuzzy_select_prompt_item(
                    item,
                    Some(idx) == sel,
                    self.highlight_matches,
                    &matcher,
                    &search_term,
                )?;
            }
            term.flush()?;

            match (term.read_key()?, sel) {
                (Key::Escape, _) if allow_quit => {
                    if self.clear {
                        render.clear()?;
                        term.flush()?;
                    }
                    term.show_cursor()?;
                    return Ok(None);
                }
                (Key::ArrowUp | Key::BackTab, _) if !filtered_list.is_empty() => {
                    if sel == Some(0) {
                        starting_row =
                            filtered_list.len().max(visible_term_rows) - visible_term_rows;
                    } else if sel == Some(starting_row) {
                        starting_row -= 1;
                    }
                    sel = match sel {
                        None => Some(filtered_list.len() - 1),
                        Some(sel) => Some(
                            ((sel as i64 - 1 + filtered_list.len() as i64)
                                % (filtered_list.len() as i64))
                                as usize,
                        ),
                    };
                    term.flush()?;
                }
                (Key::ArrowDown | Key::Tab, _) if !filtered_list.is_empty() => {
                    sel = match sel {
                        None => Some(0),
                        Some(sel) => {
                            Some((sel as u64 + 1).rem(filtered_list.len() as u64) as usize)
                        }
                    };
                    if sel == Some(visible_term_rows + starting_row) {
                        starting_row += 1;
                    } else if sel == Some(0) {
                        starting_row = 0;
                    }
                    term.flush()?;
                }
                (Key::ArrowLeft, _) if position > 0 => {
                    position -= 1;
                    term.flush()?;
                }
                (Key::ArrowRight, _) if position < search_term.len() => {
                    position += 1;
                    term.flush()?;
                }
                (Key::Enter, Some(sel)) if !filtered_list.is_empty() => {
                    if self.clear {
                        render.clear()?;
                    }

                    if self.report {
                        render
                            .input_prompt_selection(self.prompt.as_str(), filtered_list[sel].0)?;
                    }

                    let sel_string = filtered_list[sel].0;
                    let sel_string_pos_in_items =
                        self.items.iter().position(|item| item.eq(sel_string));

                    term.show_cursor()?;
                    return Ok(sel_string_pos_in_items);
                }
                (Key::Backspace, _) if position > 0 => {
                    position -= 1;
                    search_term.remove(position);
                    term.flush()?;
                }
                (Key::Char(chr), _) if !chr.is_ascii_control() => {
                    search_term.insert(position, chr);
                    position += 1;
                    term.flush()?;
                    sel = Some(0);
                    starting_row = 0;
                }

                _ => {}
            }

            render.clear_preserve_prompt(&size_vec)?;
        }
    }
}

impl<'a> FuzzySelect<'a> {
    /// Same as `new` but with a specific theme.
    pub fn with_theme(theme: &'a dyn Theme) -> Self {
        Self {
            default: None,
            items: vec![],
            prompt: "".into(),
            report: true,
            clear: true,
            highlight_matches: true,
            max_length: None,
            theme,
            initial_text: "".into(),
        }
    }
}
