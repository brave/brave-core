use super::*;
use crate::parser::state_machine::StateMachineActions;

use NonTagContentTokenOutline::*;
use TagTokenOutline::*;

// NOTE: use macro instead of the function to make borrow
// checker happy with range construction inside match arm
// with a mutable borrow of lexer.
macro_rules! get_token_part_range {
    ($self:tt) => {
        Range {
            start: $self.token_part_start,
            end: $self.next_pos - 1,
        }
    };
}

impl<S: LexemeSink> StateMachineActions for Lexer<S> {
    impl_common_sm_actions!();

    #[inline]
    fn emit_eof(&mut self, input: &[u8]) -> ActionResult {
        let lexeme = self.create_lexeme_with_raw_exclusive(input, Some(Eof));

        self.emit_lexeme(&lexeme)
    }

    #[inline]
    fn emit_text(&mut self, input: &[u8]) -> ActionResult {
        if self.pos() > self.lexeme_start {
            // NOTE: unlike any other tokens (except EOF), text tokens don't have
            // any lexical symbols that determine their bounds. Therefore,
            // representation of text token content is the raw slice.
            // Also, we always emit text if we encounter some other bounded
            // lexical structure and, thus, we use exclusive range for the raw slice.
            let lexeme =
                self.create_lexeme_with_raw_exclusive(input, Some(Text(self.last_text_type)));

            self.emit_lexeme(&lexeme)?;
        }

        Ok(())
    }

    #[inline]
    fn emit_current_token(&mut self, input: &[u8]) -> ActionResult {
        let token = self.current_non_tag_content_token.take();
        let lexeme = self.create_lexeme_with_raw_inclusive(input, token);

        self.emit_lexeme(&lexeme)
    }

    #[inline]
    fn emit_tag(&mut self, input: &[u8]) -> ActionResult {
        let token = self
            .current_tag_token
            .take()
            .expect("Tag token should exist at this point");

        let feedback = self
            .try_get_tree_builder_feedback(&token)
            .map_err(ActionError::from)?;

        let mut lexeme = self.create_lexeme_with_raw_inclusive(input, token);

        // NOTE: exit from any non-initial text parsing mode always happens on tag emission
        // (except for CDATA, but there is a special action to take care of it).
        self.set_last_text_type(TextType::Data);

        if let Some(feedback) = feedback {
            self.handle_tree_builder_feedback(feedback, &lexeme);
        }

        if let StartTag {
            ref mut ns,
            name_hash,
            ..
        } = lexeme.token_outline
        {
            self.last_start_tag_name_hash = name_hash;
            *ns = self.tree_builder_simulator.borrow().current_ns();
        }

        match self
            .emit_tag_lexeme(&lexeme)
            .map_err(ActionError::RewritingError)?
        {
            ParserDirective::Lex => Ok(()),
            ParserDirective::WherePossibleScanForTagsOnly => self.change_parser_directive(
                self.lexeme_start,
                ParserDirective::WherePossibleScanForTagsOnly,
                FeedbackDirective::None,
            ),
        }
    }

    #[inline]
    fn emit_current_token_and_eof(&mut self, input: &[u8]) -> ActionResult {
        let token = self.current_non_tag_content_token.take();
        let lexeme = self.create_lexeme_with_raw_exclusive(input, token);

        self.emit_lexeme(&lexeme)?;
        self.emit_eof(input)
    }

    #[inline]
    fn emit_raw_without_token(&mut self, input: &[u8]) -> ActionResult {
        let lexeme = self.create_lexeme_with_raw_inclusive(input, None);

        self.emit_lexeme(&lexeme)
    }

    #[inline]
    fn emit_raw_without_token_and_eof(&mut self, input: &[u8]) -> ActionResult {
        // NOTE: since we are at EOF we use exclusive range for token's raw.
        let lexeme = self.create_lexeme_with_raw_exclusive(input, None);

        self.emit_lexeme(&lexeme)?;
        self.emit_eof(input)
    }

    #[inline]
    fn create_start_tag(&mut self, _input: &[u8]) {
        self.attr_buffer.borrow_mut().clear();

        self.current_tag_token = Some(StartTag {
            name: Range::default(),
            name_hash: LocalNameHash::new(),
            ns: Namespace::default(),
            attributes: Rc::clone(&self.attr_buffer),
            self_closing: false,
        });
    }

    #[inline]
    fn create_end_tag(&mut self, _input: &[u8]) {
        self.current_tag_token = Some(EndTag {
            name: Range::default(),
            name_hash: LocalNameHash::new(),
        });
    }

    #[inline]
    fn create_doctype(&mut self, _input: &[u8]) {
        self.current_non_tag_content_token = Some(Doctype {
            name: None,
            public_id: None,
            system_id: None,
            force_quirks: false,
        });
    }

    #[inline]
    fn create_comment(&mut self, _input: &[u8]) {
        self.current_non_tag_content_token = Some(Comment(Range::default()));
    }

    #[inline]
    fn start_token_part(&mut self, _input: &[u8]) {
        self.token_part_start = self.pos();
    }

    #[inline]
    fn mark_comment_text_end(&mut self, _input: &[u8]) {
        if let Some(Comment(ref mut text)) = self.current_non_tag_content_token {
            *text = get_token_part_range!(self);
        }
    }

    #[inline]
    fn shift_comment_text_end_by(&mut self, _input: &[u8], offset: usize) {
        if let Some(Comment(ref mut text)) = self.current_non_tag_content_token {
            text.end += offset;
        }
    }

    #[inline]
    fn set_force_quirks(&mut self, _input: &[u8]) {
        if let Some(Doctype {
            ref mut force_quirks,
            ..
        }) = self.current_non_tag_content_token
        {
            *force_quirks = true;
        }
    }

    #[inline]
    fn finish_doctype_name(&mut self, _input: &[u8]) {
        if let Some(Doctype { ref mut name, .. }) = self.current_non_tag_content_token {
            *name = Some(get_token_part_range!(self));
        }
    }

    #[inline]
    fn finish_doctype_public_id(&mut self, _input: &[u8]) {
        if let Some(Doctype {
            ref mut public_id, ..
        }) = self.current_non_tag_content_token
        {
            *public_id = Some(get_token_part_range!(self));
        }
    }

    #[inline]
    fn finish_doctype_system_id(&mut self, _input: &[u8]) {
        if let Some(Doctype {
            ref mut system_id, ..
        }) = self.current_non_tag_content_token
        {
            *system_id = Some(get_token_part_range!(self));
        }
    }

    #[inline]
    fn finish_tag_name(&mut self, _input: &[u8]) -> ActionResult {
        match self.current_tag_token {
            Some(StartTag { ref mut name, .. }) | Some(EndTag { ref mut name, .. }) => {
                *name = get_token_part_range!(self)
            }
            _ => unreachable!("Tag should exist at this point"),
        }

        Ok(())
    }

    #[inline]
    fn update_tag_name_hash(&mut self, input: &[u8]) {
        if let Some(ch) = input.get(self.pos()).copied() {
            match self.current_tag_token {
                Some(StartTag {
                    ref mut name_hash, ..
                })
                | Some(EndTag {
                    ref mut name_hash, ..
                }) => name_hash.update(ch),
                _ => unreachable!("Tag should exist at this point"),
            }
        }
    }

    #[inline]
    fn mark_as_self_closing(&mut self, _input: &[u8]) {
        if let Some(StartTag {
            ref mut self_closing,
            ..
        }) = self.current_tag_token
        {
            *self_closing = true;
        }
    }

    #[inline]
    fn start_attr(&mut self, input: &[u8]) {
        // NOTE: create attribute only if we are parsing a start tag
        if let Some(StartTag { .. }) = self.current_tag_token {
            self.current_attr = Some(AttributeOutline::default());

            self.start_token_part(input);
        }
    }

    #[inline]
    fn finish_attr_name(&mut self, _input: &[u8]) {
        if let Some(AttributeOutline {
            ref mut name,
            ref mut raw_range,
            ..
        }) = self.current_attr
        {
            *name = get_token_part_range!(self);
            *raw_range = *name;
        }
    }

    #[inline]
    fn finish_attr_value(&mut self, input: &[u8]) {
        if let Some(AttributeOutline {
            ref mut value,
            ref mut raw_range,
            ..
        }) = self.current_attr
        {
            *value = get_token_part_range!(self);

            // NOTE: include closing quote into the raw value if it's present
            raw_range.end = match input.get(self.next_pos - 1).copied() {
                Some(ch) if ch == self.closing_quote => value.end + 1,
                _ => value.end,
            };
        }
    }

    #[inline]
    fn finish_attr(&mut self, _input: &[u8]) {
        if let Some(attr) = self.current_attr.take() {
            self.attr_buffer.borrow_mut().push(attr);
        }
    }

    noop_action!(mark_tag_start, unmark_tag_start);
}
