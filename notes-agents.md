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
- **Écritures** : confirmation à **chaque** écriture pour l'instant. Prochaine grande étape : les autorisations (conception terminée, voir la section « Autorisations »).
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

Tableau d'objets, ou (prévu avec les autorisations) objet `{"defaults": {...}, "agents": [...]}` :

```json
{
  "defaults": {
    "allowedCommands": ["git status", "git diff", "git log", "make *"],
    "readablePaths": ["C:/MinGW/include/"]
  },
  "agents": [{
    "name": "Local test agent",
    "api": "openai-chat",
    "url": "http://localhost:1235/v1/",
    "model": "mistralai/ministral-3-3b",
    "headers": {},
    "systemPrompt": "...",
    "temperature": 0.2,
    "maxTokens": 4096,
    "timeout": 600,
    "tools": { "*": "ask", "edit_file": "allow", "run_lua": "deny" },
    "allowedCommands": ["npm test"],
    "writablePaths": ["../shared/"],
    "maxToolIterations": 15,
    "extraBody": {}
  }]
}
```

Seuls `name` et `url` sont obligatoires. Voir la section « Autorisations » pour `tools`, `allowedCommands`, `readablePaths`, `writablePaths` et `defaults`.

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

**get_editor_context()**
- Outil seul, pas d'injection automatique dans les messages (décision : éviter le bruit dans l'historique ; VS Code / Cursor injectent le fichier actif mais de façon visible et désactivable). Une phrase du prompt système invite à l'appeler quand l'utilisateur parle du fichier courant, de la sélection, du curseur ou de « ici » (seulement si l'outil est actif).
- Retour : `Active file:`, `Unsaved changes: yes/no`, `Cursor: line L, column C`, `Selection: none` ou les bornes suivies du texte sélectionné (tronqué à `readMaxLines` lignes), puis les autres fichiers ouverts sous la racine (`(unsaved changes)` le cas échéant).
- Document sans nom : « untitled » ; hors de la racine : « outside the workspace », sans chemin ni contenu.

**open_in_editor(path, position?)**
- Ouvre (ou active) le fichier, puis `TextDocument::DoQuickJump(position)`. Pas de confirmation. Le focus n'est pas géré spécialement : c'est l'éditeur qui décide (à ajuster si ça gêne).
- `position` est un **sous-ensemble** de la syntaxe du saut rapide (`IsAllowedPosition`) : `12`, `12:5`, `[12,5]`, `12-15`, `12:5-15:20`, `/regex/` (sans `/` dans l'expression, options `i`/`s` tolérées), `#marqueur`. Exclus volontairement : `=expr` (évaluation Lua qui écrit dans le document), `$commande`, `+n`/`-n`, `^^n`, et la partie `?clé=valeur` (on appelle directement le `DoQuickJump` du document, pas celui d'`App`).
- Retour : `Opened <path> in the editor.` puis curseur et sélection résultants, ce qui permet au modèle de savoir si une recherche `/regex/` a déplacé le curseur (en cas d'échec, le saut rapide ne fait qu'un bip).

**move_file(path, new_path)**
- Fichier ou répertoire. Destination existante refusée (sauf simple changement de casse). Répertoires manquants de la destination créés. Déplacement d'un répertoire dans lui-même refusé, racine refusée.
- Les onglets ouverts suivent (`SetFilename`), y compris avec des modifications non enregistrées.

**delete_file(path)**
- Fichier ou répertoire (avec tout son contenu), toujours mis à la corbeille via `wxMoveToTrash` (wxWidgets ≥ 3.3, Windows / macOS / GTK, macro `wxHAS_MOVE_TO_TRASH` ; sinon l'outil renvoie une erreur). Racine refusée.
- Les onglets concernés sont fermés sans demander d'enregistrer ; la confirmation indique leur nombre et combien ont des modifications non enregistrées qui seront perdues. Si cela fermerait tous les documents, un document vide est créé d'abord (sinon `exit_on_last_close` fermerait l'application, et l'agent avec).
- Confirmation avec **Refuser** comme bouton par défaut (`ToolContext::Confirm(message, denyByDefault)`).

`create_directory` abandonné : `write_file` crée déjà les répertoires manquants ; un répertoire vide pourra se créer avec l'outil d'exécution de commande.

**replace_in_files(pattern, replacement, path?, regex?, ignore_case?)**
- Même sélection de fichiers que `search_files` (répertoires ignorés, binaires et extensions exclues sautés). Remplacement **sensible à la casse par défaut**, alors que `search_files` est insensible par défaut : les deux outils ont le même paramètre `ignore_case` avec des valeurs par défaut différentes, indiquées explicitement dans chaque description (chercher trop large est sans risque, remplacer trop large l'est moins ; Claude Code et Codex font la même distinction).
- Ne réutilise pas `FindAllMultiple` / `ReplaceAllMultiple` tels quels : ils se limitent à la sélection quand l'éditeur en a une, et ignorent les répertoires exclus. Réutilise `FindAllInText` (exemples avant/après) et `ReplaceAllInText`, avec la lecture/écriture communes aux autres outils.
- Hors regex, `$` et `\` du remplacement sont échappés (`pcre2_substitute` les interpréterait).
- Une seule confirmation : fichiers avec nombre de remplacements et emplacement (éditeur / disque), plus 5 exemples.
- Retour : `Replaced N occurrences in M files:` puis `path: n` par fichier.

**run_command(command, cwd?, timeout?)**
- Ligne de commande construite avec `shell` (config par agent, défaut `cmd /s /c "{command}"` ; sans `{command}`, la commande est ajoutée à la fin). Peut servir à passer par un filtre de sortie du genre rtk, si l'outil accepte la commande en arguments.
- `cwd` relatif à la racine, contrôlé par `ResolvePath` (la commande elle-même peut évidemment tout faire : ce n'est pas un bac à sable).
- API wxWidgets uniquement (une première version en Win32 direct a été abandonnée). `wxExecute` asynchrone doit être appelé sur le thread principal : lancement via `RunEDTSync` avec un `wxProcess` redirigé (`wxEXEC_HIDE_CONSOLE`, `wxExecuteEnv::cwd`), entrée fermée aussitôt (`CloseOutput`). `wxEVT_END_PROCESS` (thread principal) met à jour un état partagé (fin, code de sortie). Le thread des outils lit stdout et stderr sans bloquer (`CanRead` / `Read`, comme `LiveTextAppender` pour la console). Délai ou annulation : `wxProcess::Kill(pid, wxSIGKILL, wxKILL_CHILDREN)` sur le thread principal. Fin : `delete` du `wxProcess` sur le thread principal si la fin a été notifiée, sinon `Detach()` (il se supprime à la fin du processus).
  - stdout et stderr sont deux tubes séparés : l'ordre relatif des deux est approximatif (lus alternativement). Si ça gêne : `2>&1` dans `shell`.
  - Un programme lancé en arrière-plan par la commande n'est pas attendu : on s'arrête dès la notification de fin et quand plus rien n'est disponible dans les tubes.
- Délai : `limits.commandTimeout` (60 s) par défaut, plafonné par `limits.commandMaxTimeout` (600 s).
- Retour : `Exit code: N` en première ligne (convention de Codex et Claude Code ; c'est aussi la ligne affichée dans la conversation), puis la sortie. Décodage : UTF-8 si valide (`wxConvUTF8` est strict), sinon page de code OEM (`GetLocalOemDefaultEncoding`) ; toujours de l'UTF-8 valide vers le modèle. Troncature au milieu (`OutputBuffer` garde le début et la fin).

**run_lua(code)**
- État Lua partagé de l'application (accès à toute l'API), mais environnement propre à l'agent (`_ENV` dont `__index` = `_G`, référencé dans le registre par `LuaEnvironment`) : les globales du script persistent pendant la conversation, ne polluent pas `_G`, et `/clear` les oublie.
- Comme `LuaSimpleEval` (console) : essai en `return <code>`, sinon en instructions. Valeurs renvoyées converties avec `luaL_tolstring`, séparées par des tabulations. `print` de l'environnement capture la sortie dans une table (remis à nil après l'exécution, pour ne jamais intercepter les `print` des plugins). Erreurs avec traceback (`luaL_call`).
- Exécuté dans le thread des outils en tenant le verrou Lua ; les bindings `SYNC` passent au thread principal en relâchant le verrou. L'état Lua est créé sur le thread principal (`GetLuaState` via `RunEDTSync`).
- **À faire plus tard** : protection contre les boucles infinies (hook d'instructions `lua_sethook` qui vérifie l'annulation et un délai). Pour l'instant rien : à voir si ça pose réellement problème. Un script qui ne se termine pas garde le verrou Lua, et Shift+Échap ne l'interrompt pas.
- **À faire plus tard** : donner au modèle la référence de l'API (option B) : outil ou ressource renvoyant `scripting-reference` par section. Pour l'instant le modèle n'a qu'une description brève et l'introspection (`pairs`).

Option utilisateur pour choisir corbeille ou suppression définitive (où et comment : à décider).

## Autorisations (prochaine grande étape)

Avec `run_command` et `run_lua`, l'agent peut sortir complètement de la racine : la confirmation est le seul garde-fou. Mais confirmer chaque action devient vite pénible et pousse à cliquer « Autoriser » sans lire, ce qui est pire qu'une politique explicite.

Conception discutée et validée le 2026-11-20.

### Permissions des outils (clé `tools`)
- Valeurs : `allow` (sans confirmation), `ask` (confirmer), `deny` (outil absent), `default` (permission intégrée de l'outil).
- Chaque outil a une **permission intégrée** (`AgentTool::defaultPermission`) : `allow` pour les outils sans effet (`list_files`, `read_file`, `search_files`, `get_editor_context`, `open_in_editor`), `ask` pour les autres.
- Une seule clé `tools` (pas de clé `permissions` séparée), trois formes :
  - **absente** : tous les outils, permission intégrée ;
  - **tableau de noms** : seulement ces outils, permission intégrée (`[]` = aucun outil) ;
  - **objet** : chaque outil listé reçoit la valeur donnée. La clé `"*"` désigne les outils non listés : ceux qui ont des effets prennent sa valeur, ceux sans effet gardent toujours leur permission intégrée (`allow`), donc `{"*": "deny"}` = agent en lecture seule. Sans `"*"`, les outils non listés sont absents (liste fermée).
- Vocabulaire : `"*"` est toujours un **sélecteur** (clé = « tous les autres outils »), `default` toujours une **valeur** (« permission intégrée »). Le `"all"` de l'étape 5 est supprimé (absent = tous). Pas de compatibilité à préserver.
- `deny` ne sert qu'avec `"*"` (exclure un outil d'un ensemble) : sinon il suffit de ne pas lister l'outil.
- Un outil `deny` ou absent n'est pas envoyé au modèle ; s'il est appelé quand même : `Error:` indiquant qu'il est désactivé par la configuration. Les messages de refus distinguent toujours « refusé par la configuration » et « refusé par l'utilisateur » (pour que le modèle ne réessaie pas en boucle).

### Garde-fous (même en `allow`)
- **`run_command`** : une commande qui correspond à `allowedCommands` s'exécute sans confirmation ; les autres demandent toujours confirmation. Seul un `allow` **sur la clé `run_command` elle-même** lève ce garde-fou (`"*": "allow"` ne suffit pas). À expliquer clairement dans la doc (usage prévu : VM, bac à sable).
- **`run_lua`** : même règle (toujours confirmer, sauf `allow` explicite sur sa propre clé). Pas de liste blanche possible : le code est arbitraire et a accès à `os.execute`, `io.open`...
- **Chemins hors racine** : aujourd'hui refusés ; désormais :
  - dans la racine, ou couverts par `readablePaths` (lecture) / `writablePaths` (lecture et écriture) : permission normale de l'outil ;
  - sinon : confirmation **toujours** demandée, quelle que soit la permission de l'outil.
  - S'applique à tous les outils qui prennent un chemin, y compris `cwd` de `run_command`.
- Refuser reste le bouton par défaut pour `delete_file`, `run_command`, `run_lua`. Pas d'autre garde-fou fixe.

### Liste blanche de commandes (`allowedCommands`)
- Style glob : `*` = n'importe quels caractères (espaces compris), `?` = un caractère. Casse ignorée.
- Ancrage au début implicite, fin ouverte, mais coupure sur une limite de mot : `git status` correspond à `git status -s`, pas à `git statusx`.
- Comparée à la commande donnée par le modèle (espaces de début et de fin retirés), pas à la ligne construite avec `shell`.
- **Jamais d'approbation automatique** si la commande contient `&`, `|`, `<`, `>`, `^`, `%` ou un saut de ligne (sinon `git status & del /s /q *` passerait). Plus tard : découper sur `&&`, `||`, `&`, `|` et vérifier chaque partie.

### Chemins hors racine (`readablePaths`, `writablePaths`)
- Globs (`GlobToRegEx`) ; une entrée sans joker ou finissant par `/` = le dossier et tout son contenu. Entrée relative = résolue depuis la racine de l'agent (ex. `../shared/`). `writablePaths` implique la lecture.
- Comparaison après canonicalisation (`..`), normalisation en `/`, casse ignorée.
- Le modèle voit **toujours** `/` : relatif dans la racine (`src/App.cpp`), absolu en dehors (`C:/MinGW/include/stdio.h`), UNC en `//serveur/partage/...`. En entrée, `/` et `\` acceptés (arguments des outils et config). Attention : un `/` initial seul reste la racine de l'espace de travail, mais `//` (UNC) et `X:/` sont absolus.
- `list_files` / `search_files` sur un dossier hors racine renvoient des chemins absolus.
- `ResolvePath` passe de deux états (accepté / refusé) à trois : dans la racine, hors racine autorisé, hors racine à confirmer (avec distinction lecture / écriture).

### Dialogue de confirmation
- Boutons : **Autoriser** (une fois), **Autoriser pour la session**, **Toujours autoriser**, **Refuser**, **Arrêter l'agent**.
- Portée de « session » / « toujours » selon la raison de la demande :

| Raison | Portée | « Toujours » écrit dans |
|---|---|---|
| outil en `ask` (`edit_file`...) | l'outil entier | `tools` de l'agent (`"outil": "allow"`) |
| commande hors liste blanche | motif **modifiable** dans le dialogue, pré-rempli avec le programme et la sous-commande (`git diff`) | `allowedCommands` de l'agent |
| chemin hors racine | dossier concerné, modifiable | `readablePaths` ou `writablePaths` de l'agent |
| `run_lua`, `delete_file` | « Autoriser » une fois seulement (pas de session ni de toujours) | — |

- Autorisations de session gardées dans `AgentSession` jusqu'à la fermeture de l'agent (`/clear` ne les oublie pas).
- « Toujours » écrit dans l'entrée de l'agent dans `agents.json` (retrouvée par son nom), jamais dans `defaults`. Réécriture avec `nlohmann::ordered_json` pour garder l'ordre des clés ; les commentaires sont perdus (à dire dans la doc). Si `tools` est un tableau ou absent, il est d'abord converti en objet (`["a","b"]` → `{"a":"default","b":"default"}`, absent → `{"*":"default"}`).
- Un motif de commande créé par « session » / « toujours » suit les mêmes règles que `allowedCommands` (une commande composée demande toujours).

### Valeurs communes (`defaults`)
- `agents.json` accepte un tableau (comme aujourd'hui) ou `{"defaults": {...}, "agents": [...]}`. Toute clé d'agent est permise dans `defaults` (`shell`, `limits`, `timeout`...).
- Fusion (petite fonction maison : `merge_patch` de nlohmann remplace les tableaux au lieu de les concaténer) :
  - objets (`limits`, `headers`, `tools` objet) : fusion récursive, la valeur de l'agent gagne ;
  - `allowedCommands`, `readablePaths`, `writablePaths` : concaténation ;
  - tout le reste, y compris `tools` sous forme de tableau : la valeur de l'agent remplace.

### Plan d'implémentation
1. `defaults` et fusion dans `AgentConfig`.
2. Nouvelle forme de `tools`, `AgentTool::defaultPermission`, calcul de la permission effective, outils absents non envoyés.
3. `allowedCommands` : correspondance glob, limite de mot, refus des métacaractères.
4. `ResolvePath` à trois états, `readablePaths` / `writablePaths`, chemins absolus en `/` dans les sorties.
5. Dialogue de confirmation à 5 boutons, champ de motif modifiable, autorisations de session.
6. « Toujours autoriser » : réécriture d'`agents.json`.
7. Doc `doc/agents.md` et `config/agents.example.json`.

Pistes pour plus tard :
- Découpage des commandes composées pour la liste blanche.
- Chemins interdits même dans la racine (`.git/`, `.env`...).

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
  - Les outils s'exécutaient d'abord sur le thread principal ; ils ont un thread à eux depuis l'étape 5 (voir plus bas).
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
  - [x] `get_editor_context`, `open_in_editor`, `move_file`, `delete_file` (compilés ; à tester dans l'interface).
  - Dans la conversation, les outils qui modifient des fichiers n'affichent que `path` et `new_path`.
  - À vérifier en test : en mode SDI/MDI, `GetCurrentDocument()` pourrait être nul quand le dialogue de l'agent a le focus (la fenêtre enfant désactive sa vue). En mode onglets (défaut) ça devrait aller. Sinon, mémoriser le dernier document actif.
    Si aucun document actif n'est trouvé, `get_editor_context` le dit simplement (pas d'erreur) et la boucle continue.
  - Corrigé dans le saut rapide (`qjFind`) : l'option `i` rendait la recherche sensible à la casse (`FRI_ICASE` = sensible). Désormais `/x/` est sensible et `/x/i` insensible, comme dans `doc/quickJump.md`.
  - `tools` accepte `"all"` (chaîne, ou tableau contenant `"all"`) ; absent = tous les outils, `[]` = aucun. (`"all"` sera supprimé avec les autorisations, voir cette section.) Un outil non activé appelé quand même par le modèle est refusé (`AgentSession::HandleToolCalls`), pas exécuté.
  - [x] Documentation utilisateur : `doc/agents.md` (→ `agents.html` via `make doc`, inclus dans le zip par `z.bat`, ainsi que `config/agents.example.json`). **À compléter à chaque nouvel outil ou paramètre de configuration** ; n'y décrire que ce qui est implémenté.
  - [x] Statistiques de tokens (compilé ; à tester dans l'interface) : champ `usage` standard de `chat/completions` (`prompt_tokens`, `completion_tokens`, `prompt_tokens_details.cached_tokens` ; acceptés aussi : `input_tokens`/`output_tokens`, `prompt_cache_hit_tokens` de DeepSeek). Cumul sur toutes les requêtes d'un échange (`TokenUsage`, `AgentSession::GetLastUsage`), ligne « Tokens: … » après la réponse. Rien n'est affiché si le serveur ne renvoie pas `usage` ; la partie cache seulement si le serveur la fournit. Pas affiché en cas d'erreur ou d'annulation. Commande `/tokens` : totaux depuis le début de la conversation (`AgentSession::GetTotalUsage`), y compris les échanges échoués, annulés ou retirés par `/undo` (tokens réellement consommés), remis à zéro par `/clear` ; plus la taille du contexte à la dernière requête (`TokenUsage::lastInput`).
  - [x] `replace_in_files`, `run_command`, `run_lua` (compilés ; à tester dans l'interface). Voir les fiches ci-dessus.
  - [x] Outils exécutés dans un thread séparé (compilé ; à tester) :
    - Un `std::thread` détaché par lot d'appels d'outils d'une réponse (`AgentSession::HandleToolCalls`), plutôt que le worker : le worker est unique et partagé (aperçu de Rechercher/Remplacer, marqueurs), une commande de 2 minutes le bloquerait.
    - `ToolContext` est copié dans le thread (limites par valeur, plus de référence vers la session). Annulation par `std::shared_ptr<std::atomic<bool>> cancelled`, nouveau pour chaque échange ; Shift+Échap le met à vrai : le thread s'arrête après l'outil en cours (une commande en cours est tuée). Les résultats reviennent par `CallAfter`, seulement si `alive` est toujours vrai ; `OnToolCallsDone` poursuit la boucle.
    - `AgentSession::IsBusy()` = requête HTTP en cours ou outils en cours.
    - Accès aux documents et à l'éditeur uniquement sur le thread principal : `IsOpenInEditor`, `GetOpenFiles` (un seul aller-retour pour `search_files` / `replace_in_files`), `ReadTextFile`, `WriteTextFile` passent par `RunEDTSync`. Les outils surtout graphiques (`get_editor_context`, `open_in_editor`, `move_file`, `delete_file`) ont `AgentTool::mainThread` et s'exécutent entièrement via `RunEDTSync`.
    - Confirmations via `RunEDTSync` ; le callback vérifie que la session existe encore (sinon « Arrêter »).
    - Confirmation avec texte long (`ShowConfirmDetailsDialog` dans `AgentDialog.cpp`) : message, zone de texte en lecture seule qui a le focus, Autoriser / Refuser / Arrêter l'agent. Refuser par défaut pour `run_command` et `run_lua`.
  - `search_files` a maintenant un paramètre `ignore_case` (défaut true).
  - Corrigé au passage : `luabinding.hpp` utilisait `std::variant` sans inclure `<variant>`.
- [ ] Autorisations : conception validée (section « Autorisations »), implémentation selon le plan en 7 points.
  - [x] 1. `defaults` et fusion (`MergeDefaults` dans `AgentConfig.cpp`, compilé ; à tester). La concaténation des trois listes blanches est déjà en place ; la doc ne la décrira qu'avec les listes elles-mêmes.
