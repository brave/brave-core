use super::handlers_dispatcher::{ContentHandlersDispatcher, SelectorHandlersLocator};
use super::{HandlerTypes, RewritingError};
use crate::html::{LocalName, Namespace};
use crate::rewritable_units::{DocumentEnd, Token, TokenCaptureFlags};
use crate::selectors_vm::{AuxStartTagInfoRequest, ElementData, SelectorMatchingVm, VmError};
use crate::transform_stream::*;
use hashbrown::HashSet;

#[derive(Default)]
pub(crate) struct ElementDescriptor {
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

pub(crate) struct HtmlRewriteController<'h, H: HandlerTypes> {
    handlers_dispatcher: ContentHandlersDispatcher<'h, H>,
    selector_matching_vm: Option<SelectorMatchingVm<ElementDescriptor>>,
}

impl<'h, H: HandlerTypes> HtmlRewriteController<'h, H> {
    #[inline]
    pub(crate) const fn new(
        handlers_dispatcher: ContentHandlersDispatcher<'h, H>,
        selector_matching_vm: Option<SelectorMatchingVm<ElementDescriptor>>,
    ) -> Self {
        HtmlRewriteController {
            handlers_dispatcher,
            selector_matching_vm,
        }
    }
}

impl<H: HandlerTypes> HtmlRewriteController<'_, H> {
    #[inline]
    fn respond_to_aux_info_request(
        aux_info_req: AuxStartTagInfoRequest<ElementDescriptor, SelectorHandlersLocator>,
    ) -> StartTagHandlingResult<Self> {
        Err(DispatcherError::InfoRequest(Box::new(
            move |this, aux_info| {
                if let Some(ref mut vm) = this.selector_matching_vm {
                    let mut match_handler = |m| this.handlers_dispatcher.start_matching(m);

                    aux_info_req(vm, aux_info, &mut match_handler)
                        .map_err(RewritingError::MemoryLimitExceeded)?;
                }

                Ok(this.get_capture_flags())
            },
        )))
    }

    #[inline]
    fn get_capture_flags(&self) -> TokenCaptureFlags {
        self.handlers_dispatcher.get_token_capture_flags()
    }
}

impl<H: HandlerTypes> TransformController for HtmlRewriteController<'_, H> {
    #[inline]
    fn initial_capture_flags(&self) -> TokenCaptureFlags {
        self.get_capture_flags()
    }

    fn handle_start_tag(
        &mut self,
        local_name: LocalName<'_>,
        ns: Namespace,
    ) -> StartTagHandlingResult<Self> {
        match self.selector_matching_vm {
            Some(ref mut vm) => {
                let mut match_handler = |m| self.handlers_dispatcher.start_matching(m);

                match vm.exec_for_start_tag(local_name, ns, &mut match_handler) {
                    Ok(()) => Ok(self.get_capture_flags()),
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

    fn handle_end_tag(&mut self, local_name: LocalName<'_>) -> TokenCaptureFlags {
        if let Some(ref mut vm) = self.selector_matching_vm {
            vm.exec_for_end_tag(local_name, |elem_desc| {
                self.handlers_dispatcher.stop_matching(elem_desc);
            });
        }

        self.get_capture_flags()
    }

    #[inline]
    fn handle_token(&mut self, token: &mut Token<'_>) -> Result<(), RewritingError> {
        let current_element_data = self
            .selector_matching_vm
            .as_mut()
            .and_then(SelectorMatchingVm::current_element_data_mut);

        self.handlers_dispatcher
            .handle_token(token, current_element_data)
            .map_err(RewritingError::ContentHandlerError)
    }

    fn handle_end(&mut self, document_end: &mut DocumentEnd<'_>) -> Result<(), RewritingError> {
        self.handlers_dispatcher
            .handle_end(document_end)
            .map_err(RewritingError::ContentHandlerError)
    }

    #[inline]
    fn should_emit_content(&self) -> bool {
        !self
            .handlers_dispatcher
            .has_matched_elements_with_removed_content()
    }
}
