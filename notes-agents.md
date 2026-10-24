# Agents IA : plan et décisions

Notes de travail pour l'ajout d'agents IA dans l'éditeur. À relire avant de reprendre le travail.

## Scénario utilisateur

1. F11 ou menu Outils > Agent IA... → `App::OnOpenAgentDialog()`.
2. Choix de l'agent parmi ceux configurés (SingleChoiceDialog ; sauté s'il n'y en a qu'un).
3. Un onglet / une fenêtre s'ouvre (AgentDocument, AgentView, AgentEditor). Titre = nom de l'agent.
4. L'utilisateur tape son prompt et l'envoie avec Shift+Enter (`AgentEditor::OnShiftEnter()` → `AgentView::Send()`).
5. Requête HTTP via `WebRequester`, réponse complète ajoutée par `AgentEditor::WriteAnswer()`.
6. La conversation continue ; elle est perdue à la fermeture.
7. Plusieurs agents peuvent être ouverts simultanément, chacun avec son propre contexte.

## Décisions prises

- **Racines accessibles** : ensemble des racines (`GetWorkspaceRoot()`) des documents texte ouverts, **recalculé à chaque appel d'outil**. Permet de travailler sur plusieurs projets à la fois. Contrôle strict des chemins : canonicalisation, refus de tout ce qui sort des racines.
- **Écritures** : confirmation à **chaque** écriture pour l'instant. Plus tard : option globale ou par agent (`permissions`: `ask` / `auto` / `readonly`).
- **Fichier déjà ouvert** : lecture ET écriture dans le contenu de l'éditeur (`FindEditor()`, `ReplaceAndPushUndoState`), sans enregistrement automatique. Fichier fermé : lecture / écriture sur disque (`LoadFile` / `SaveFile` + editorconfig). La confirmation indique lequel des deux cas s'applique.
- **Pas de streaming** : on attend la réponse complète.
- **Markdown** affiché tel quel. Plus tard peut-être une vue web.
- **Configuration** dans `config/agents.json` (application portable). Plus tard éventuellement : recherche dans %APPDATA% d'abord, et variables d'environnement dans la config avec la syntaxe `${env:NOM}`. Pas prioritaire tant qu'on n'utilise pas de vraies clés API.
- API : format OpenAI `chat/completions` (champ `api` prévu pour d'autres formats plus tard, ex. Anthropic).
- JSON : nlohmann (déjà présent dans le projet).

## Points d'attention

- Serveur de test : http://localhost:1235/v1/ (modèle `mistralai/ministral-3-3b`), **très lent**. Le délai de réception WinHTTP par défaut (30 s) est trop court → délai configurable (`timeout`).
- Pendant une requête : état « en cours », Shift+Enter bloqué, annonces `SayText` au départ et à l'arrivée (même si l'utilisateur est sur un autre onglet), annulation par Échap (étape 2).
- Onglet fermé pendant une requête : ne pas toucher à une vue détruite (annuler la requête ou vérifier la validité).
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
| `edit_file(path, old, new, replace_all?)` | remplacement exact | `ReplaceAndPushUndoState` |
| `write_file(path, content)` | créer / écraser | `SaveFile` |

Plus tard : remplacement multifichier (`ReplaceAllMultiple`, avec confirmation), `get_editor_context` (fichier actif, ligne, sélection), `open_in_editor(path, line)`, déplacer / supprimer (confirmation), exécuter une commande (beaucoup plus tard).

## Commandes et menus prévus (étape 2)

`/clear`, `/model [nom]` (sans argument : liste via `GET /models`), `/retry`, `/undo`, `/stop`, `/system`, `/roots`, `/tokens`, `/save`, `/help`.
Menus : effacer (Ctrl+Shift+L), changer de modèle, annuler (Échap), copier la dernière réponse, navigation entre messages (ex. Alt+Haut / Alt+Bas).

## Architecture

- `AgentConfig` : lecture de `config/agents.json`.
- `AgentSession` : historique (JSON), modèle courant, envoi des requêtes, plus tard boucle d'outils et annulation. Appartient à la vue.
- `AgentTools` (étape 3) : registre nom + schéma JSON + handler, avec un contexte (racines, callback de confirmation).
- Interface (AgentView / AgentEditor) séparée de la logique.

## Étapes

1. **Conversation simple sans outils** : config, choix de l'agent, requête `chat/completions`, timeout, état en cours, annonces, erreurs affichées, protection fermeture d'onglet.
2. Commandes, menus, annulation (Échap), navigation entre messages.
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
- [ ] Étape 3
- [ ] Étape 4
- [ ] Étape 5
