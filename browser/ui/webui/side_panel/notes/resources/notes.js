import {
  PageCallbackRouter,
  PageHandlerFactory,
  PageHandlerRemote,
} from './notes.mojom-webui.js';

const titleInput = document.querySelector('#note-title');
const contentInput = document.querySelector('#note-content');
const form = document.querySelector('#note-form');
const notesElement = document.querySelector('#notes');

let editingId = null;
let handler = null;

async function initialize() {
  const callbackRouter = new PageCallbackRouter();
  callbackRouter.onNotesChanged.addListener(render);
  handler = new PageHandlerRemote();
  const factory = PageHandlerFactory.getRemote();
  factory.createPageHandler(
      callbackRouter.$.bindNewPipeAndPassRemote(),
      handler.$.bindNewPipeAndPassReceiver());
  const notes = await handler.getNotes();
  render(notes.notes);
}

function render(notes) {
  notesElement.replaceChildren();
  for (const note of notes) {
    const article = document.createElement('article');
    article.className = 'note';
    article.innerHTML =
        '<strong></strong><p></p><div class="note-actions">' +
        '<button type="button" data-action="edit">Edit</button>' +
        '<button type="button" data-action="delete">Delete</button></div>';
    article.querySelector('strong').textContent = note.title;
    article.querySelector('p').textContent = note.content;

    article.querySelector('[data-action="edit"]').addEventListener(
        'click', () => {
          editingId = note.id;
          titleInput.value = note.title;
          contentInput.value = note.content;
        });
    article.querySelector('[data-action="delete"]').addEventListener(
        'click', () => handler.deleteNote(note.id));
    notesElement.append(article);
  }
}

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const note = {
    id: editingId || crypto.randomUUID(),
    title: titleInput.value,
    content: contentInput.value,
    timestamp: Date.now(),
  };
  if (editingId) {
    await handler.updateNote(note);
  } else {
    await handler.addNote(note);
  }
  editingId = null;
  form.reset();
});

initialize();
