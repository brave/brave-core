use super::settings::*;
use super::ElementDescriptor;
use crate::rewritable_units::{DocumentEnd, Element, StartTag, Token, TokenCaptureFlags};
use crate::selectors_vm::MatchInfo;

#[derive(Copy, Clone, Default, Debug, PartialEq, Eq, Hash)]
pub(crate) struct SelectorHandlersLocator {
    pub element_handler_idx: Option<usize>,
    pub comment_handler_idx: Option<usize>,
    pub text_handler_idx: Option<usize>,
}

struct HandlerVecItem<H> {
    handler: H,
    user_count: usize,
}

struct HandlerVec<H> {
    items: Vec<HandlerVecItem<H>>,
    user_count: usize,
}

impl<H> Default for HandlerVec<H> {
    fn default() -> Self {
        Self {
            items: Vec::default(),
            user_count: 0,
        }
    }
}

impl<H> HandlerVec<H> {
    #[inline]
    pub fn push(&mut self, handler: H, always_active: bool) {
        let item = HandlerVecItem {
            handler,
            user_count: usize::from(always_active),
        };

        self.user_count += item.user_count;
        self.items.push(item);
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.items.len()
    }

    #[inline]
    pub fn inc_user_count(&mut self, idx: usize) {
        self.items[idx].user_count += 1;
        self.user_count += 1;
    }

    #[inline]
    pub fn dec_user_count(&mut self, idx: usize) {
        self.items[idx].user_count -= 1;
        self.user_count -= 1;
    }

    #[inline]
    pub const fn has_active(&self) -> bool {
        self.user_count > 0
    }

    #[inline]
    pub fn for_each_active(
        &mut self,
        mut cb: impl FnMut(&mut H) -> HandlerResult,
    ) -> HandlerResult {
        for item in &mut self.items {
            if item.user_count > 0 {
                cb(&mut item.handler)?;
            }
        }

        Ok(())
    }

    #[inline]
    pub fn do_for_each_active_and_deactivate(
        &mut self,
        mut cb: impl FnMut(&mut H) -> HandlerResult,
    ) -> HandlerResult {
        for item in &mut self.items {
            if item.user_count > 0 {
                cb(&mut item.handler)?;
                self.user_count -= item.user_count;
                item.user_count = 0;
            }
        }

        Ok(())
    }

    #[inline]
    pub fn do_for_each_active_and_remove(
        &mut self,
        mut cb: impl FnMut(H) -> HandlerResult,
    ) -> HandlerResult {
        for i in (0..self.items.len()).rev() {
            if self.items[i].user_count > 0 {
                let item = self.items.remove(i);

                self.user_count -= item.user_count;

                cb(item.handler)?;
            }
        }

        Ok(())
    }
}

pub(crate) struct ContentHandlersDispatcher<'h, H: HandlerTypes> {
    doctype_handlers: HandlerVec<H::DoctypeHandler<'h>>,
    comment_handlers: HandlerVec<H::CommentHandler<'h>>,
    text_handlers: HandlerVec<H::TextHandler<'h>>,
    end_tag_handlers: HandlerVec<H::EndTagHandler<'static>>,
    element_handlers: HandlerVec<H::ElementHandler<'h>>,
    end_handlers: HandlerVec<H::EndHandler<'h>>,
    next_element_can_have_content: bool,
    matched_elements_with_removed_content: usize,
}

impl<H: HandlerTypes> Default for ContentHandlersDispatcher<'_, H> {
    fn default() -> Self {
        ContentHandlersDispatcher {
            doctype_handlers: Default::default(),
            comment_handlers: Default::default(),
            text_handlers: Default::default(),
            end_tag_handlers: Default::default(),
            element_handlers: Default::default(),
            end_handlers: Default::default(),
            next_element_can_have_content: false,
            matched_elements_with_removed_content: 0,
        }
    }
}

impl<'h, H: HandlerTypes> ContentHandlersDispatcher<'h, H> {
    #[inline]
    pub fn add_document_content_handlers(&mut self, handlers: DocumentContentHandlers<'h, H>) {
        if let Some(handler) = handlers.doctype {
            self.doctype_handlers.push(handler, true);
        }

        if let Some(handler) = handlers.comments {
            self.comment_handlers.push(handler, true);
        }

        if let Some(handler) = handlers.text {
            self.text_handlers.push(handler, true);
        }

        if let Some(handler) = handlers.end {
            self.end_handlers.push(handler, true);
        }
    }

    #[inline]
    pub fn add_selector_associated_handlers(
        &mut self,
        handlers: ElementContentHandlers<'h, H>,
    ) -> SelectorHandlersLocator {
        SelectorHandlersLocator {
            element_handler_idx: handlers.element.map(|h| {
                self.element_handlers.push(h, false);
                self.element_handlers.len() - 1
            }),
            comment_handler_idx: handlers.comments.map(|h| {
                self.comment_handlers.push(h, false);
                self.comment_handlers.len() - 1
            }),
            text_handler_idx: handlers.text.map(|h| {
                self.text_handlers.push(h, false);
                self.text_handlers.len() - 1
            }),
        }
    }

    #[inline]
    pub const fn has_matched_elements_with_removed_content(&self) -> bool {
        self.matched_elements_with_removed_content > 0
    }

    #[inline]
    pub fn start_matching(&mut self, match_info: MatchInfo<SelectorHandlersLocator>) {
        let locator = match_info.payload;

        if match_info.with_content {
            if let Some(idx) = locator.comment_handler_idx {
                self.comment_handlers.inc_user_count(idx);
            }

            if let Some(idx) = locator.text_handler_idx {
                self.text_handlers.inc_user_count(idx);
            }
        }

        if let Some(idx) = locator.element_handler_idx {
            self.element_handlers.inc_user_count(idx);
        }

        self.next_element_can_have_content = match_info.with_content;
    }

    #[inline]
    pub fn stop_matching(&mut self, elem_desc: ElementDescriptor) {
        for locator in elem_desc.matched_content_handlers {
            if let Some(idx) = locator.comment_handler_idx {
                self.comment_handlers.dec_user_count(idx);
            }

            if let Some(idx) = locator.text_handler_idx {
                self.text_handlers.dec_user_count(idx);
            }
        }

        if let Some(idx) = elem_desc.end_tag_handler_idx {
            self.end_tag_handlers.inc_user_count(idx);
        }

        if elem_desc.remove_content {
            self.matched_elements_with_removed_content -= 1;
        }
    }

    pub fn handle_start_tag(
        &mut self,
        start_tag: &mut StartTag<'_>,
        current_element_data: Option<&mut ElementDescriptor>,
    ) -> HandlerResult {
        if self.matched_elements_with_removed_content > 0 {
            start_tag.remove();
        }

        let mut element = Element::new(start_tag, self.next_element_can_have_content);

        self.element_handlers
            .do_for_each_active_and_deactivate(|h| h(&mut element))?;

        if self.next_element_can_have_content {
            if let Some(elem_desc) = current_element_data {
                if element.should_remove_content() {
                    elem_desc.remove_content = true;
                    self.matched_elements_with_removed_content += 1;
                }

                if let Some(handler) = element.into_end_tag_handler() {
                    elem_desc.end_tag_handler_idx = Some(self.end_tag_handlers.len());

                    self.end_tag_handlers.push(handler, false);
                }
            }
        }

        Ok(())
    }

    pub fn handle_token(
        &mut self,
        token: &mut Token<'_>,
        current_element_data: Option<&mut ElementDescriptor>,
    ) -> HandlerResult {
        match token {
            Token::Doctype(doctype) => self.doctype_handlers.for_each_active(|h| h(doctype)),
            Token::StartTag(start_tag) => self.handle_start_tag(start_tag, current_element_data),
            Token::EndTag(end_tag) => self
                .end_tag_handlers
                .do_for_each_active_and_remove(|h| h(end_tag)),
            Token::TextChunk(text) => self.text_handlers.for_each_active(|h| h(text)),
            Token::Comment(comment) => self.comment_handlers.for_each_active(|h| h(comment)),
        }
    }

    pub fn handle_end(&mut self, document_end: &mut DocumentEnd<'_>) -> HandlerResult {
        self.end_handlers
            .do_for_each_active_and_remove(|h| h(document_end))
    }

    #[inline]
    pub fn get_token_capture_flags(&self) -> TokenCaptureFlags {
        let mut flags = TokenCaptureFlags::empty();

        if self.doctype_handlers.has_active() {
            flags |= TokenCaptureFlags::DOCTYPES;
        }

        if self.comment_handlers.has_active() {
            flags |= TokenCaptureFlags::COMMENTS;
        }

        if self.text_handlers.has_active() {
            flags |= TokenCaptureFlags::TEXT;
        }

        if self.end_tag_handlers.has_active() {
            flags |= TokenCaptureFlags::NEXT_END_TAG;
        }

        if self.element_handlers.has_active() {
            flags |= TokenCaptureFlags::NEXT_START_TAG;
        }

        flags
    }
}
