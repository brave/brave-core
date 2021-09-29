export function addWebUIListener(eventName: string, handler: Function) {}
export function sendWithPromise(message: string, ...args: any[]): Promise<any> { return new Promise(resolve => console.log('sendWithPromise', message, args)) }
