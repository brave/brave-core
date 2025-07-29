
namespace web {
class WebState;
}  // namespace web

@interface AIChatCommunicationTabHelper()
- (void)CreateForWebState:(web::WebState)webState;
+ (nullable AIChatCommunicationController*)FromWebState:(web::WebState*)webState;
@end
