use super::handlers_dispatcher::{ContentHandlersDispatcher, SelectorHandlersLocator};
use super::RewritingError;
use crate::html::{LocalName, Namespace};
use crate::rewritable_units::{DocumentEnd, Token, TokenCaptureFlags};
use crate::selectors_vm::{AuxStartTagInfoRequest, ElementData, SelectorMatchingVm, VmError};
use crate::transform_stream::*;
use hashbrown::HashSet;
use std::cell::RefCell;
use std::rc::Rc;

#[derive(Default)]
pub struct ElementDescriptor {
    pub matched_content_handlers: HashSet<SelectorHandlersLocator>,
    pub end_tag_handler_idx: Option<usize>,
    pub remove_content: bool,
}

impl ElementData for ElementDescriptor {
    type MatchPayload = SelectorHandlersLocator;

    #[inline]
    fn matched_payload_mut(&mut self) -> &mut HashSet<SelectorHandlersLocator> {
        &mut self.matched_content_handlers
    }
}

pub struct HtmlRewriteController<'h> {
    handlers_dispatcher: Rc<RefCell<ContentHandlersDispatcher<'h>>>,
    selector_matching_vm: Option<SelectorMatchingVm<ElementDescriptor>>,
}

impl<'h> HtmlRewriteController<'h> {
    #[inline]
    pub fn new(
        handlers_dispatcher: ContentHandlersDispatcher<'h>,
        selector_matching_vm: Option<SelectorMatchingVm<ElementDescriptor>>,
    ) -> Self {
        HtmlRewriteController {
            handlers_dispatcher: Rc::new(RefCell::new(handlers_dispatcher)),
            selector_matching_vm,
        }
    }
}

// NOTE: it's a macro instead of an instance method, so it can be executed
// when we hold a mutable reference for the selector matching VM.
macro_rules! create_match_handler {
    ($self:tt) => {{
        let handlers_dispatcher = Rc::clone(&$self.handlers_dispatcher);

        move |m| handlers_dispatcher.borrow_mut().start_matching(m)
    }};
}

impl<'h> HtmlRewriteController<'h> {
    #[inline]
    fn respond_to_aux_info_request(
        aux_info_req: AuxStartTagInfoRequest<ElementDescriptor, SelectorHandlersLocator>,
    ) -> StartTagHandlingResult<Self> {
        Err(DispatcherError::InfoRequest(Box::new(
            move |this, aux_info| {
                let mut match_handler = create_match_handler!(this);

                if let Some(ref mut vm) = this.selector_matching_vm {
                    aux_info_req(vm, aux_info, &mut match_handler)
                        .map_err(RewritingError::MemoryLimitExceeded)?;
                }

                Ok(this.get_capture_flags())
            },
        )))
    }

    #[inline]
    fn get_capture_flags(&self) -> TokenCaptureFlags {
        self.handlers_dispatcher.borrow().get_token_capture_flags()
    }
}

impl TransformController for HtmlRewriteController<'_> {
    #[inline]
    fn initial_capture_flags(&self) -> TokenCaptureFlags {
        self.get_capture_flags()
    }

    fn handle_start_tag(
        &mut self,
        local_name: LocalName,
        ns: Namespace,
    ) -> StartTagHandlingResult<Self> {
        match self.selector_matching_vm {
            Some(ref mut vm) => {
                let mut match_handler = create_match_handler!(self);

                match vm.exec_for_start_tag(local_name, ns, &mut match_handler) {
                    Ok(_) => Ok(self.get_capture_flags()),
                    Err(VmError::InfoRequest(req)) => Self::respond_to_aux_info_request(req),
                    Err(VmError::MemoryLimitExceeded(e)) => Err(DispatcherError::RewritingError(
                        RewritingError::MemoryLimitExceeded(e),
                    )),
                }
            }
            // NOTE: fast path - we can skip executing selector matching VM completely
            // and don't need to maintain open element stack if we don't have any selectors.
            None => Ok(self.get_capture_flags()),
        }
    }

    fn handle_end_tag(&mut self, local_name: LocalName) -> TokenCaptureFlags {
        if let Some(ref mut vm) = self.selector_matching_vm {
            let handlers_dispatcher = Rc::clone(&self.handlers_dispatcher);

            vm.exec_for_end_tag(local_name, move |elem_desc| {
                handlers_dispatcher.borrow_mut().stop_matching(elem_desc);
            });
        }

        self.get_capture_flags()
    }

    #[inline]
    fn handle_token(&mut self, token: &mut Token) -> Result<(), RewritingError> {
        let current_element_data = self
            .selector_matching_vm
            .as_mut()
            .and_then(SelectorMatchingVm::current_element_data_mut);

        self.handlers_dispatcher
            .borrow_mut()
            .handle_token(token, current_element_data)
            .map_err(RewritingError::ContentHandlerError)
    }

    fn handle_end(&mut self, document_end: &mut DocumentEnd) -> Result<(), RewritingError> {
        self.handlers_dispatcher
            .borrow_mut()
            .handle_end(document_end)
            .map_err(RewritingError::ContentHandlerError)
    }

    #[inline]
    fn should_emit_content(&self) -> bool {
        !self
            .handlers_dispatcher
            .borrow()
            .has_matched_elements_with_removed_content()
    }
}
