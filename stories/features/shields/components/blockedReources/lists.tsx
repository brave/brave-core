// export const enabledList = (list: any[]) => {
//   return list.map((item, index) => {
//     if (item.blocked === false) {
//       return null
//     }
//     return (
//       <ResourcesListGrid key={index}>
//         {
//           item.hasUserInput
//           ? <ResourcesListBlockedLink>{locale.blocked}</ResourcesListBlockedLink>
//           : <Link onClick={this.onClickAllowItem}>{locale.allow}</Link>
//         }
//         <ResourcesListItem>{item.name}</ResourcesListItem>
//         {
//           item.hasUserInput
//           ? <HiddenLink onClick={this.onClickUndoAction}>{locale.undo}</HiddenLink>
//           : null
//         }
//       </ResourcesListGrid>
//     )
//   })
// }

// export const disabledList = (list: any[]) => {
//   return list.map((item, index) => {
//     if (item.blocked === true) {
//       return null
//     }
//     return (
//       <ResourcesListGrid key={index}>
//         {
//           item.hasUserInput
//           ? <ResourcesListAllowedLink>{locale.allowed}</ResourcesListAllowedLink>
//           : <Link onClick={this.onClickBlockItem}>{locale.block}</Link>
//         }
//         <ResourcesListItem>{item.name}</ResourcesListItem>
//         {
//           item.hasUserInput
//           ? <HiddenLink onClick={this.onClickUndoAction}>{locale.undo}</HiddenLink>
//           : null
//         }
//       </ResourcesListGrid>
//     )
//   })
// }
