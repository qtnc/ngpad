# Agents IA : plan et décisions

Notes de travail pour l'ajout d'agents IA dans l'éditeur. À relire avant de reprendre le travail.

## Scénario utilisateur

1. F11 ou menu Outils > Agent IA... → `App::OnOpenAgentDialog()`.
2. La racine est celle du document actif (`App::ChooseAgentRoot()`). Si un agent existe déjà pour cette racine, son dialogue est réaffiché tel quel.
3. Sinon, choix de l'agent parmi ceux configurés (SingleChoiceDialog ; sauté s'il n'y en a qu'un), puis ouverture d'un dialogue non modal (`AgentDialog`, `AgentEditor`). Titre = « Nom de l'agent (dossier) ».
4. L'utilisateur tape son prompt et l'envoie avec Shift+Enter (`AgentEditor::OnShiftEnter()` → `AgentDialog::Send()`).
5. Requête HTTP via `WebRequester`, réponse complète ajoutée par `AgentEditor::WriteAnswer()`.
6. Échap (ou la case de fermeture) masque seulement le dialogue : la conversation et une requête en cours sont conservées. F11 y revient. « Fermer l'agent » (menu contextuel) le détruit et perd la conversation.
7. Un seul agent par racine ; plusieurs racines = plusieurs dialogues, chacun avec son propre contexte.

## Décisions prises

- **Dialogue non modal plutôt qu'onglet** (décision révisée ; l'agent était d'abord un type de document comme la console). Le document actif de la fenêtre principale reste celui que l'utilisateur regarde, ce qui servira à `get_editor_context` : on pourra le lire à chaque envoi.
- **Un agent par racine**, racine fixe pour toute sa durée de vie : `GetWorkspaceRoot()` du document actif s'il a un nom de fichier, sinon sélecteur de répertoire. Retrouvé par `App::FindAgent()` (parcours de `wxTopLevelWindows`, comparaison insensible à la casse). Pour travailler sur deux projets, on ouvre deux agents. Meilleure isolation entre projets.
  - Le modèle ne voit que des chemins relatifs à la racine, avec `/` comme séparateur. En entrée on accepte aussi `\`, un `/` initial (= racine de l'espace de travail, jamais la racine du disque) et les chemins absolus situés sous la racine. Tout ce qui sort de la racine est refusé (canonicalisation, `..`).
- **Écritures** : confirmation à **chaque** écriture pour l'instant. Plus tard : option globale ou par agent (`permissions`: `ask` / `auto` / `readonly`).
- **Fichier déjà ouvert** : lecture ET écriture dans le contenu de l'éditeur (`FindEditor()`, `ReplaceAndPushUndoState`), sans enregistrement automatique. Fichier fermé : lecture / écriture sur disque (`LoadFile` / `SaveFile` + editorconfig). La confirmation indique lequel des deux cas s'applique.
- **Pas de streaming** : on attend la réponse complète.
- **Markdown** affiché tel quel. Plus tard peut-être une vue web.
- **Configuration** dans `config/agents.json` (application portable). Plus tard éventuellement : recherche dans %APPDATA% d'abord, et variables d'environnement dans la config avec la syntaxe `${env:NOM}`. Pas prioritaire tant qu'on n'utilise pas de vraies clés API.
- API : format OpenAI `chat/completions` (champ `api` prévu pour d'autres formats plus tard, ex. Anthropic).
- JSON : nlohmann (déjà présent dans le projet).

## Points d'attention

- Serveur de test : http://localhost:1235/v1/ (modèle `mistralai/ministral-3-3b`), **très lent**. Le délai de réception WinHTTP par défaut (30 s) est trop court → délai configurable (`timeout`).
- Pendant une requête : état « en cours », Shift+Enter bloqué, annonces `SayText` au départ et à l'arrivée (même si le dialogue est masqué), annulation par Shift+Échap (Échap seul masque le dialogue).
- Agent fermé pendant une requête : ne pas toucher à un dialogue détruit (annuler la requête ou vérifier la validité).
- Confirmation d'écriture alors que le dialogue est masqué : il est réaffiché avant la question.
- Boucle d'outils : un prompt peut produire plusieurs requêtes ; limiter le nombre de tours (`maxToolIterations`) et afficher chaque appel d'outil en une ligne.
- Petit modèle (3B) : peu d'outils (~5), schémas simples.
- `edit_file` (remplacement exact) est préférable à `write_file` : moins de tokens, et échoue proprement si l'utilisateur a modifié le passage entre-temps.

## Format de configuration (`config/agents.json`)

Tableau d'objets :

```json
{
  "name": "Local test agent",
  "api": "openai-chat",
  "url": "http://localhost:1235/v1/",
  "model": "mistralai/ministral-3-3b",
  "headers": {},
  "systemPrompt": "...",
  "temperature": 0.2,
  "maxTokens": 4096,
  "timeout": 600,
  "tools": ["list_files", "read_file", "search", "edit_file", "write_file"],
  "permissions": "ask",
  "maxToolIterations": 15,
  "extraBody": {}
}
```

Seuls `name` et `url` sont obligatoires. `tools`, `permissions`, `maxToolIterations` : étapes 3-4.

## Outils prévus

| Outil | Rôle | Réutilise |
|---|---|---|
| `list_files(glob, root?)` | chemins relatifs, nombre limité, ignore `.git`, `obj`... | `FindAllFiles` |
| `read_file(path, start?, end?)` | numéros de ligne, tronqué au-delà d'une limite | `FindEditor` / `LoadFile` |
| `search(pattern, glob, regex?)` | résultats `fichier:ligne:texte` | `FindReplaceInfo::FindAllMultiple` |
| `edit_file(path, old_text, new_text, replace_all?)` | remplacement exact | `ReplaceAndPushUndoState` |
| `write_file(path, content)` | créer / écraser | `SaveFile` |

### Fiches détaillées (brouillon à valider)

Règles communes :
- Chemins relatifs à la racine, séparateur `/`. Répertoires ignorés : liste configurable dans `config.ini` (ex. `agent_ignored_dirs=.git, .svn, .hg, node_modules, obj, objd, __pycache__`). Fichiers binaires ignorés (`search_files`) ou refusés (`read_file`).
- Masques : `*`, `?`, `**` via `GlobToRegEx` (déjà en place). Chemins normalisés en `/` avant comparaison, sinon un masque contenant `/` ne correspond jamais aux chemins Windows.
- Un succès ne commence **jamais** par `Error:` ; une erreur commence **toujours** par `Error: ` et indique quoi faire ensuite. Résultat vide = phrase explicite, jamais une chaîne vide.
- Toutes les limites sont configurables par agent (valeurs par défaut ci-dessous), plus une limite globale de taille appliquée à tout résultat d'outil, y compris aux futurs outils Lua :
  `"limits": { "readMaxLines": 500, "listMaxEntries": 500, "searchMaxResults": 100, "searchMaxLineLength": 200, "maxResultChars": 50000 }`

**list_files(path?, recursive?)**
- Description : "List files and directories in the workspace. Directories end with '/'. Paths are relative to the workspace root. Not recursive unless recursive is true or the pattern contains '**'."
- `path` (string, optionnel, défaut `""` = racine) : répertoire, éventuellement suivi d'un masque : `src/`, `src/*.cpp`, `src/**/*.hpp`.
- `recursive` (boolean, optionnel, défaut false) ; devient vrai automatiquement si le masque contient `**`.
- Retour : une entrée par ligne, répertoires d'abord puis fichiers, tri alphabétique insensible à la casse. En mode récursif, seulement les fichiers. Troncature : `[Truncated: N more entries not shown. Use a more specific path or pattern.]`. Vide : `No files match: <path>`.

**read_file(path, start_line?, end_line?)**
- Description : "Read a text file from the workspace. Returns a header line followed by the requested lines, each prefixed by its line number and a tab. Line numbers are not part of the file content. Long files are truncated: use start_line and end_line to read other parts."
- `path` (string, obligatoire).
- `start_line`, `end_line` (integer, optionnels) : numérotés à partir de 1, `end_line` inclus, négatif = depuis la fin (-1 = dernière ligne). Bornes au-delà de la fin ramenées à la fin.
- Lecture via le contenu de l'éditeur si le fichier est ouvert, sinon `LoadFile` (détection d'encodage). Retour toujours en UTF-8.
- Retour : `File: src/app/App.cpp (lines 1-500 of 1534)` puis `1\t...`. Troncature : `[Truncated at 500 lines. Use start_line=501 to continue.]`. Fichier vide : `File: <path> (empty)`.
- Erreurs : introuvable, répertoire, hors racine, binaire, `start_line` au-delà de la fin.

**search_files(pattern, path?, regex?)**
- Description : "Search for text in workspace files, case-insensitively. The search is always recursive. Returns one line per match in the form path:line: text."
- `pattern` (string, obligatoire).
- `path` (string, optionnel, défaut `""`) : même syntaxe que `list_files`.
- `regex` (boolean, optionnel, défaut false) : expression régulière PCRE.
- Réutilise `FindReplaceInfo::FindAllMultiple` (contenu de l'éditeur pour les fichiers ouverts), avec `FRI_ICASE`.
- Retour : `src/app/App.cpp:92: texte`, lignes coupées à `searchMaxLineLength` avec `…`. Ligne finale `[42 matches in 7 files]`. Troncature : `[N more matches not shown. Use a more specific path or pattern.]`. Vide : `No matches for '<pattern>' in <path>`.

**edit_file(path, old_text, new_text, replace_all?)**
- Remplacement exact (comparaison sur le texte normalisé en `\n`, aucune tolérance sur les espaces). Une seule occurrence exigée sauf `replace_all: true` (convention établie : Claude Code, Codex, Aider) ; 0 occurrence ou plusieurs sans `replace_all` = erreur. `old_text` vide → erreur renvoyant vers `write_file`.
- Pas d'option `regex` en v1 ; à ajouter plus tard (même booléen que `search_files`, substitutions `$1`…`$9`, même règle d'unicité).
- Retour : `Edited <path>: N replacement(s).`

**write_file(path, content)**
- Fichier ouvert : tout le contenu de l'éditeur est remplacé (annulable). Fichier fermé existant : encodage et fins de ligne conservés (`LoadFile` + editorconfig). Nouveau fichier : editorconfig puis `default_charset` / `default_line_ending` de `config.ini` ; répertoires manquants créés ; pas d'ouverture dans un onglet.
- Retour : `Wrote <path>: N line(s).`

Communs aux deux outils d'écriture :
- Le modèle ne sait pas si le fichier est ouvert ou non (lecture et recherche lisent déjà l'éditeur, donc vue cohérente).
- Confirmation : `wxMessageDialog` Autoriser / Refuser / Arrêter l'agent ; le message donne le fichier et l'endroit (éditeur sans enregistrer / disque). Pas de `SayText`. Refuser → erreur renvoyée au modèle, la boucle continue. Arrêter (ou Échap dans la confirmation) → comme une annulation : l'échange est retiré de l'historique, les appels restants abandonnés (les modifications déjà autorisées restent dans les fichiers).
- Dans la conversation, seul l'argument `path` est affiché (`AgentTool::modifiesFiles`). Chaque appel d'outil est écrit en entier dans `debug.txt` via `println`.

Plus tard : remplacement multifichier (`ReplaceAllMultiple`, avec confirmation), `get_editor_context` (fichier actif, ligne, sélection), `open_in_editor(path, line)`, déplacer / supprimer (confirmation), exécuter une commande (beaucoup plus tard).

## Commandes et menus prévus (étape 2)

`/clear`, `/model [nom]` (sans argument : liste via `GET /models`), `/retry`, `/undo`, `/stop`, `/system`, `/roots`, `/tokens`, `/save`, `/help`.
Menu contextuel (pas de barre de menus dans le dialogue) : copier, tout sélectionner, enregistrer (Ctrl+S), effacer (Ctrl+Shift+L), changer de modèle (Ctrl+M), annuler (Shift+Échap), fermer l'agent. Navigation entre messages (Ctrl+Haut / Ctrl+Bas).

## Architecture

- `AgentConfig` : lecture de `config/agents.json`.
- `AgentSession` : historique (JSON), modèle courant, envoi des requêtes, boucle d'outils et annulation. Appartient au dialogue.
- `AgentTools` (étape 3) : registre nom + schéma JSON + handler, avec un contexte (racines, callback de confirmation). **Registre dynamique, pas une liste figée** : beaucoup plus tard, l'utilisateur pourra ajouter ses propres outils en Lua ; les outils intégrés s'enregistrent par le même mécanisme. Un handler renvoie un résultat {erreur oui/non, texte} indépendant du format d'API.
- Conventions des outils : noms et paramètres en anglais, snake_case, de la forme verbe_nom (`read_file`, `list_files`, `search_files`...), mêmes noms de paramètres d'un outil à l'autre (`path` partout). Descriptions et prompt système en anglais ; consigne explicite de répondre dans la langue de l'utilisateur (éventuellement ajouter la langue de l'interface au prompt système).
- Le modèle d'embedding listé par `/models` reste visible (pas de filtrage par nom).
- Interface (AgentDialog / AgentEditor) séparée de la logique.

## Étapes

1. **Conversation simple sans outils** : config, choix de l'agent, requête `chat/completions`, timeout, état en cours, annonces, erreurs affichées, protection fermeture d'onglet.
2. Commandes, menus, annulation, navigation entre messages.
3. Outils en lecture seule : `list_files`, `read_file`, `search`.
4. Outils d'écriture avec confirmation : `edit_file`, `write_file`.
5. Compléments (outils supplémentaires, variables d'environnement, recherche de config avancée, permissions configurables).

## Avancement

- [x] Étape 1 (compilée, requête vérifiée avec curl ; test dans l'interface à faire par l'utilisateur)
  - `src/agent/AgentConfig.*` : lecture de `config/agents.json` via `App::FindAppFile` (cherche déjà dans le dossier utilisateur %APPDATA% puis à côté de l'exécutable). Commentaires autorisés dans le JSON.
  - `src/agent/AgentSession.*` : historique, `POST {url}chat/completions`, timeout (`SetTimeouts`), erreurs HTTP/JSON, retrait du prompt sans réponse de l'historique. Un `shared_ptr<bool>` (`alive`) protège le callback si l'onglet est fermé pendant la requête.
  - `AgentView` possède la session ; pendant une requête la saisie est bloquée (bip + annonce « L'agent n'a pas encore répondu »).
  - `config/agents.json` est ignoré par git ; `config/agents.example.json` est versionné.
- [x] Étape 2 (compilée ; test dans l'interface à faire par l'utilisateur)
  - Menus spécifiques dans `AgentDocument::AddSpecificMenus` (menu Édition) : effacer (Ctrl+Shift+L), choisir le modèle (Ctrl+M, SingleChoiceDialog), annuler la requête (Échap). Traités dans `AgentView::OnMenuAction`.
  - Commandes : `/clear`, `/model` (liste), `/model <nom>` (OK / non trouvé + liste), `/help`. Le résultat d'une commande s'écrit sans en-tête, dans le bloc du message qui la contient. Pas de boîte de dialogue pour `/model`.
  - Liste des modèles (`GET {url}models`) en cache par URL, partagée entre les onglets.
  - Navigation Ctrl+Haut/Bas : début du contenu du message précédent/suivant (ligne après l'en-tête). Ctrl+Shift+Bas sélectionne jusqu'à la fin du message (lignes vides finales exclues), Ctrl+Shift+Haut jusqu'au début du précédent. Les en-têtes sont mémorisés par position (`messageMarks`), pas détectés par le texte.
  - Effacer pendant une requête : la requête est abandonnée sans écrire « Requête annulée » dans la nouvelle conversation.
  - Écartés : `/stop` (inutile car la saisie est bloquée pendant une requête ; Échap le remplace) et « copier la dernière réponse » (Ctrl+Shift+Bas puis Ctrl+C suffit).
  - `/undo` : retire le dernier échange réussi (message + réponse) de l'historique ET de la zone de texte, ainsi que tout ce qui a été écrit après (sorties de commandes, erreurs, la commande `/undo` elle-même). Répétable. La vue mémorise la position de début de chaque échange réussi (`exchangeStarts`).
  - `/retry` : comme `/undo`, puis réécrit le même message dans la zone de saisie et le renvoie.
  - Pas d'items de menu pour `/undo` et `/retry` pour l'instant.
- [x] Étape 3 (compilée ; format des requêtes/réponses avec outils vérifié avec curl contre le serveur local ; outils eux-mêmes à tester dans l'interface)
  - Choix de la racine : `App::ChooseAgentRoot()`. Titre de l'onglet : « Nom de l'agent (dossier) ».
  - `src/agent/AgentTools.*` : registre (`AgentTools::Register`, outils intégrés enregistrés au premier accès), `ToolContext` (racine, limites, résolution et contrôle des chemins), `list_files`, `read_file`, `search_files`. Les handlers C++ s'appellent `ToolListFiles`, etc. (`ReadFile` entre en conflit avec l'API Windows).
  - `AgentSession` : boucle d'outils (`HandleToolCalls`), au plus `maxToolIterations` tours ; en cas d'échec ou d'annulation, tout l'échange (message utilisateur, appels et résultats d'outils) est retiré de l'historique. Une phrase indiquant le nom de la racine est ajoutée au prompt système quand des outils sont actifs.
  - Config : `tools` (absent = tous les outils), `maxToolIterations`, `limits` ; `agent_ignored_dirs` dans `config.ini` ; `workspace_file_list_exclude_exts` (déjà existant) est réutilisé pour ignorer des fichiers dans `search_files`.
  - Chaque appel d'outil est écrit sur une ligne dans la conversation (« Outil read_file (path: ...): première ligne du résultat ») et annoncé brièvement (« Outil read_file »).
  - Les outils s'exécutent sur le thread principal : une recherche sur une très grosse arborescence peut bloquer l'interface un moment. À déplacer sur le worker si ça pose problème.
  - Attention : dans `FindReplaceInfo`, `FRI_ICASE` signifie en fait « sensible à la casse » (sans ce drapeau, la recherche est insensible).
  - Corrigé au passage : `GlobToRegEx` testait `glob[i]` au lieu de `glob[i+1]` pour `[!...]`.
- [x] Étape 4 (compilée ; à tester dans l'interface)
  - `edit_file` et `write_file` dans `AgentTools.cpp` (`ToolEditFile`, `ToolWriteFile`, écriture commune `WriteTextFile`).
  - `ToolContext::confirm` (callback fourni par `AgentView` via `AgentSession::SetConfirmCallback` ; absent = refus). `ToolResult::stop` interrompt la boucle d'outils (`FailExchange`).
  - Corrigé au passage : `SaveFile` lisait `text[-1]` sur un texte vide avec `insert_final_newline`.
- [x] Passage en dialogue non modal (compilé ; à tester dans l'interface)
  - `AgentView` / `AgentDocument` remplacés par `AgentDialog` (`wxDIALOG_NO_PARENT`, parent = fenêtre principale) ; le modèle de document « Agent » est supprimé.
  - Raccourcis par table d'accélérateurs du dialogue (Ctrl+Shift+L, Ctrl+M, Ctrl+S) ; Échap et Shift+Échap dans `AgentDialog::OnCharHook`.
  - Enregistrement de la conversation : Ctrl+S, menu contextuel ou `/save` (UTF-8, `.md` par défaut, proposé dans la racine).
  - F6 (`IDM_SWITCH_PANE`) ne sert qu'à passer de l'éditeur au panneau latéral dans `TextView` : rien pour passer d'une fenêtre flottante à l'autre. Le dialogue apparaît dans la barre des tâches, donc Alt+Tab fonctionne.
- [ ] Étape 5
