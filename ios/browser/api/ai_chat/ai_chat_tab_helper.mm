

AIChatTabHelper::AIChatTabHelper(web::WebState* web_state)
    : web::WebStateObserver(web_state),
      web::WebStateUserData<AIChatTabHelper>(*web_state),
      AssociatedContentDriverIOS(web_state->GetBrowserState()
                                        ->GetSharedURLLoaderFactory()),
      page_content_fetcher_delegate_(
          std::make_unique<PageContentFetcher>(web_state)) {
  previous_page_title_ = web_state->GetTitle();
}

AIChatTabHelper::~AIChatTabHelper() = default;




WEB_STATE_USER_DATA_KEY_IMPL(AIChatTabHelper)
