# Gestion de fichiers dans l'arbre du projet (Ctrl+P) : plan

Plan autonome, prêt à être repris dans une nouvelle conversation. Rien n'est encore codé. Toutes les décisions de conception ci-dessous ont été validées par l'utilisateur ; il ne reste pas de question ouverte. Avancer étape par étape (section « Étapes »), en cochant la section « Avancement » au fur et à mesure.

## Objectif

1. Transformer l'arbre des fichiers du projet (Ctrl+P) en petit gestionnaire de fichiers : renommer, couper/copier/coller (compatible avec l'Explorateur Windows), supprimer (corbeille), créer un fichier ou un répertoire vide, copier le chemin complet.
2. Mettre en commun avec les outils d'agent `move_file` et `delete_file` le code qui déplace et supprime, pour que l'interface et l'agent appellent les mêmes fonctions.
3. Dans un second temps, passer par git quand le fichier est suivi (`git mv`, mise à jour de l'index après une suppression). Grâce à la mise en commun, l'agent en profite aussi.

**Hors périmètre** : l'arbre des marqueurs du fichier courant (F6, `TreeMarkerJumpList`). Ne pas y toucher. Idées pour plus tard en fin de document.

## Contexte du projet

- Éditeur de texte wxWidgets 3.3 (MSW), C++17, MinGW. Exécutable `NGPad.exe` (release) / `NGPadd.exe` (debug).
- Compilation : `c.bat` (debug, `mingw32-make`), `r.bat` (release). Le `makefile` compile tous les `.cpp` de `src/common`, `src/app`, `src/console`, `src/text`, `src/agent`, `src/lua/...` (un nouveau fichier dans ces répertoires est pris automatiquement).
- Tests dans l'interface faits par l'utilisateur. Toujours compiler avant de rendre la main.
- Traductions : `lang/main_fr.properties` (clé=texte, `{}` pour les paramètres avec `format`). Dans le code : `MSG("Cle")` (wxString) ou `GetTranslation("Cle")` (std::string). Une clé absente est affichée telle quelle. Voir les clés `Agent...` existantes comme modèle.
- Annonces au lecteur d'écran : `wxGetApp().SayText(texte)`.
- Style du code existant (à respecter) : pas d'indentation dans les corps de fonction, accolade ouvrante sur la même ligne, espace avant la parenthèse dans les déclarations (`void Foo (int x) {`), gardes d'en-tête `#ifndef _____NOM_HPP`, conversions `U(...)` entre `std::string` (UTF-8) et `wxString`, `format(...)` (fmt).
- Plan des agents IA : `notes-agents.md`. Ce chantier-ci peut passer avant les étapes restantes de ce plan ; il ne modifie que l'implémentation interne de deux outils.

## État actuel du code

### Arbre Ctrl+P

- `App::ShowFileTree()` (`src/app/App.cpp`, commande `IDM_FILE_TREE`) crée un `TreeJumpListDialog` unique (pointeur `static`), avec pour racine `GetWorkspaceRoot()` du document actif ; ensuite `SetRootDir()` + `Show()`.
- `TreeJumpListDialog` (`src/text/TreeJumpListDialog.*`) : dialogue non modal (`wxDIALOG_NO_PARENT`), champ de filtre (glob ; une majuscule ajoute un `*` implicite devant elle ; rafraîchi par un timer 1 s après la frappe ; Entrée = activer), un `TreeJumpList`, boutons OK (activer) et Annuler (`Hide()`).
- `AddFileTree()` (même fichier) : `FindAllFiles` récursif, vide et reconstruit tout l'arbre, exclut éventuellement des extensions (`workspace_file_list_exclude_exts`, `workspace_file_list_hide_excluded`), sélectionne toujours le premier fichier. Chaque fichier porte son chemin absolu dans un `StringTreeItemData`. Les nœuds de répertoires sont créés par `MakeFileTreeItem` (défini dans `src/text/FindReplaceDialog.cpp`, aussi utilisé par l'arbre des résultats de recherche) et n'ont **aucune donnée**. Les répertoires vides n'apparaissent pas (l'arbre est déduit des chemins de fichiers).
- `TreeJumpList` (`src/text/TreeJumpList.*`, 16 lignes) : `wxTreeCtrl` dont l'activation lit le `StringTreeItemData` et appelle `App::DoQuickJump(chaîne)`. Aussi utilisé par `TreeMarkerJumpList` (F6) : **ne pas le modifier**.

### Outils d'agent (`src/agent/AgentTools.cpp`)

- `ToolMoveFile` : résolution des chemins (`ToolContext::ResolvePath`, interdit de sortir de la racine), validations (racine, existence, source = destination, changement de casse seul autorisé, destination existante, répertoire déplacé en lui-même), confirmation (`context.Confirm`), `wxFileName::Mkdir` du parent, `wxRenameFile`, puis les documents ouverts suivent (`doc->SetFilename(nouveau, true)`, modifications non enregistrées conservées), `UpdateTitle()`.
- `ToolDeleteFile` : validations, message de confirmation avec le nombre de fichiers d'un répertoire, le nombre de documents ouverts et non enregistrés (`AgentConfirmDeleteFile`, `AgentConfirmDeleteDir`, `AgentConfirmDeleteUnsaved`, `AgentConfirmDeleteOpen`), `wxMoveToTrash` (sous `#ifdef wxHAS_MOVE_TO_TRASH`), puis `CloseDocuments`.
- Utilitaires `static` sans lien avec l'agent : `NormalizePath`, `FindOpenDocuments(path, isDir)` (documents dont le fichier est ce chemin ou se trouve dessous, comparaison insensible à la casse), `CloseDocuments(docs)` (ferme sans demander d'enregistrer ; crée d'abord un document vide si on ferme le dernier, sinon l'application quitterait).

## Décisions de conception

- **Nouveau composant `FileTree`**, dérivé directement de `wxTreeCtrl`, complètement séparé de `TreeJumpList` (pas d'héritage). Données structurées sur chaque élément. On fusionnera plus tard si du code commun apparaît.
- **Déplacement** : uniquement couper/coller (glisser-déposer peut-être plus tard).
- **Renommage** : édition en place dans l'arbre.
- **Création** : un nœud est ajouté dans l'arbre et son nom est saisi par édition en place, pas dans une boîte de dialogue. Noms proposés : « nouveau.txt » et « Nouveau dossier » (traduits, rendus uniques). Le nouveau fichier n'est pas ouvert automatiquement.
- **Suppression** : toujours confirmée, bouton par défaut Non. Corbeille. Les onglets des fichiers supprimés sont fermés.
- **Collage et copie** : pas de confirmation.
- **Nom déjà pris au collage** : suffixe numérique (voir « Noms uniques »).
- **Pas de nœud racine visible** : coller ou créer avec le focus sur un fichier utilise son répertoire parent.
- **Presse-papiers système**, compatible avec l'Explorateur dans les deux sens, uniquement avec les classes de wxWidgets (pas d'appel win32 direct).
- **Raccourcis** (actifs quand l'arbre a le focus) et **menu contextuel** avec les mêmes actions.
- **Git** : `git mv` pour une source suivie ; suppression par la corbeille puis `git rm --cached`. Rien pour la copie et la création.

## Conception

### A. Opérations communes : `src/app/FileOperations.hpp/.cpp`

Dans `src/app` car le module dépend de `App` et des documents.

```cpp
enum class FileOpError { None, NotFound, SameFile, DestinationExists, MoveIntoItself, InvalidName, NotSupported, Failed };

struct FileOpInfo {          // pour construire une confirmation
bool isDir = false;
size_t fileCount = 0;        // fichiers contenus (répertoire)
size_t openDocs = 0, unsavedDocs = 0;
};

namespace FileOperations {
FileOpError CheckMove (const wxString& source, const wxString& dest);
FileOpError Move (const wxString& source, const wxString& dest);   // mkdir parent, renommage, documents ouverts suivent
FileOpError CheckCopy (const wxString& source, const wxString& dest);
FileOpError Copy (const wxString& source, const wxString& dest);   // récursif pour un répertoire
FileOpError CheckDelete (const wxString& path);
FileOpError Delete (const wxString& path);                         // corbeille, documents fermés
FileOpError CreateFile (const wxString& path);                     // fichier vide
FileOpError CreateDir (const wxString& path);
FileOpInfo Describe (const wxString& path);
wxString MakeUniqueName (const wxString& path);
bool IsValidName (const wxString& name);
wxString NormalizePath (const wxString& path);
std::vector<AbstractDocument*> FindOpenDocuments (const wxString& path, bool isDir);
void CloseDocuments (const std::vector<AbstractDocument*>& docs);
}
```

Principes :

- **Codes d'erreur, pas de messages** : l'agent formule des messages en anglais pour le modèle, l'interface des messages traduits.
- **Pas de confirmation dans ce module** : l'agent garde `ToolContext::Confirm`, l'interface utilise `wxMessageBox`. `Describe` fournit les mêmes informations aux deux.
- **`Check*` avant confirmation, et revalidation dans l'opération** (le disque a pu changer pendant la question).
- `CheckMove` et `CheckCopy` refusent aussi la copie d'un répertoire dans lui-même ou un de ses sous-répertoires.
- La restriction à la racine du projet reste propre à l'agent (`ResolvePath`), pas dans ce module.
- `CreateFile` / `CreateDir` créent les répertoires parents manquants.
- `IsValidName` : non vide, pas de `/` ni `\`, pas de caractère de `wxFileName::GetForbiddenChars()`, pas `.` ni `..`.
- **Notification** : après chaque opération réussie, appeler un point d'entrée de l'application (par exemple `App::OnFilesChanged()`) qui demande au `FileTree` ouvert de se reconstruire. Ainsi une opération de l'agent se voit dans l'arbre.

**Noms uniques** (`MakeUniqueName`) : si le chemin n'existe pas, le renvoyer tel quel. Sinon, découper le nom en *base* + *nombre final éventuel* + *extension* (`^(.*?)(\d*)$` sur le nom sans extension) et essayer *base* + N + *extension*, N croissant à partir de nombre+1 (ou 2 s'il n'y a pas de nombre), jusqu'à un nom libre.

- `fichier.ext` → `fichier2.ext`, puis `fichier3.ext`...
- `fichier2.ext` → `fichier3.ext` (pas `fichier22.ext`).
- `rapport2024.txt` → `rapport2025.txt` : effet de bord connu et accepté (règle simple et prévisible préférée à des heuristiques).
- Sans extension : `Makefile` → `Makefile2`. Nom commençant par un point (`.gitignore`) : le nom entier sert de base → `.gitignore2`.
- Répertoires : même règle (`Nouveau dossier` → `Nouveau dossier2`).

### B. Composant `src/text/FileTree.hpp/.cpp`

```cpp
struct FileTreeItemData: wxTreeItemData {
wxString path;          // absolu, normalisé
bool isDir;
bool pending = false;   // nœud temporaire en cours de création
};

class FileTree: public wxTreeCtrl {
// styles : wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_EDIT_LABELS | wxTR_SINGLE
public:
void SetRootDir (const wxString& dir);
void SetFilter (const wxString& glob);
void Rebuild (const wxString& pathToSelect = wxEmptyString);
wxString GetSelectedPath () const;
wxString GetTargetDir () const;   // répertoire sélectionné, ou parent du fichier sélectionné, ou racine
bool OpenSelected ();
void Rename (); void Cut (); void Copy (); void Paste (); void Delete ();
void NewFile (); void NewDir (); void CopyPath ();
};
```

- **Construction** : sa propre version de `AddFileTree` / `MakeFileTreeItem` (ne pas toucher à celles de `FindReplaceDialog`), qui attache un `FileTreeItemData` à chaque répertoire. Garder le même filtrage (glob, extensions exclues). Lister aussi les répertoires vides, mais seulement sans filtre actif.
- **`Rebuild(pathToSelect)`** : reconstruit puis sélectionne l'élément de ce chemin s'il existe (en dépliant ses parents), sinon le premier fichier comme aujourd'hui.
- **Activation** (Entrée, double-clic) d'un fichier : `wxGetApp().OpenOrCreateDocument(path)` puis `doc->Activate()` et `UpdateTitle()`. Sur un répertoire : comportement natif (déplier/replier).
- **Dialogue** : `TreeJumpListDialog` remplace son `TreeJumpList` par un `FileTree` (garder le filtre, les boutons, `SetRootDir`). Le bouton OK et Entrée dans le filtre appellent `OpenSelected()`. Le renommer en `FileTreeDialog` au passage (et mettre à jour `App::ShowFileTree`). Rafraîchir l'arbre quand le dialogue est réactivé (`wxEVT_ACTIVATE`), pour voir les modifications faites ailleurs.

#### Raccourcis et menu contextuel

| Action | Touche | Sur un fichier | Sur un répertoire |
|---|---|---|---|
| Ouvrir | Entrée | Ouvre dans un onglet | Déplie/replie |
| Renommer | F2 | Édition en place | Idem |
| Couper | Ctrl+X | Presse-papiers (déplacement), annonce | Idem |
| Copier | Ctrl+C | Presse-papiers (copie), annonce | Idem |
| Coller | Ctrl+V | Dans le répertoire du fichier | Dans ce répertoire |
| Supprimer | Suppr | Confirmation, corbeille, onglet fermé | Confirmation (nombre de fichiers), corbeille, onglets fermés |
| Nouveau fichier | Ctrl+N | Dans le répertoire du fichier | Dans ce répertoire |
| Nouveau répertoire | Ctrl+Shift+N | Idem | Idem |
| Copier le chemin complet | Ctrl+Shift+C | Chemin absolu en texte | Idem |
| Menu contextuel | Applications, Shift+F10, clic droit | Toutes les actions ci-dessus | Idem |

- Raccourcis gérés par le composant lui-même (`wxEVT_KEY_DOWN`), donc seulement quand l'arbre a le focus : Ctrl+C / Ctrl+V dans le champ de filtre restent le copier/coller de texte.
- Menu contextuel : `wxEVT_TREE_ITEM_MENU` (couvre clic droit et touche Applications), qui appelle les mêmes méthodes. Libellés traduits, avec le raccourci après une tabulation.
- Annoncer le résultat de chaque action avec `SayText` (« Coupé : x », « Copié : x », « x collé », etc.).

#### Renommer

F2 → `EditLabel`. Dans `wxEVT_TREE_END_LABEL_EDIT` :

- édition annulée (`IsEditCancelled()`) ou nom inchangé : rien ;
- nom invalide (`IsValidName`) ou erreur de `CheckMove` / `Move` (même répertoire, nouveau nom) : `Veto()`, message d'erreur, puis relancer l'édition (`CallAfter` → `EditLabel`) pour corriger sans tout retaper ;
- succès : `Veto()` aussi (l'arbre sera reconstruit), puis `CallAfter(Rebuild(nouveauChemin))`. Ne pas reconstruire l'arbre pendant le traitement de l'événement.

#### Créer un fichier ou un répertoire

1. Ajouter un nœud temporaire (`pending = true`) dans le répertoire cible, nommé `MakeUniqueName(cible + "nouveau.txt")` ou `"Nouveau dossier"` (clés de traduction). Le sélectionner, déplier son parent, `EditLabel`.
2. Édition validée : `CreateFile` / `CreateDir` avec le nom saisi, puis `Rebuild` en sélectionnant le nouveau chemin (ordre alphabétique retrouvé).
3. Édition annulée : supprimer le nœud temporaire. Rien n'est créé sur le disque.
4. Nom invalide ou existant : `Veto()`, message, relancer l'édition.

#### Presse-papiers (compatible avec l'Explorateur)

Tout est dans wxWidgets 3.3 (vérifié dans `C:\MinGW\ProgQ\WXWidgets\include\wx`) :

- **`wxFileDataObject`** (`AddFile`, `GetFilenames`) : format `wxDF_FILENAME`, c'est-à-dire la liste de fichiers que l'Explorateur écrit sur Ctrl+C et lit sur Ctrl+V.
- **Couper** : l'Explorateur distingue couper et copier par le format nommé « Preferred DropEffect », un entier de 4 octets (1 = copier, 2 = déplacer). L'écrire avec `wxCustomDataObject(wxDataFormat("Preferred DropEffect"))` ; le constructeur de `wxDataFormat` à partir d'un nom enregistre le format.
- **Écriture** (Ctrl+X / Ctrl+C) : `wxDataObjectComposite` contenant le `wxFileDataObject` et le `wxCustomDataObject`, puis `wxTheClipboard->Open()`, `SetData`, `Close()`.
- **Lecture** (Ctrl+V) : `IsSupported(wxDF_FILENAME)` puis `GetData` d'un `wxFileDataObject` ; ensuite, si `IsSupported(wxDataFormat("Preferred DropEffect"))`, `GetData` d'un `wxCustomDataObject` pour savoir s'il faut déplacer. Deux lectures séparées (un composite ne reçoit qu'un format).
- **Après le collage de fichiers coupés** : `wxTheClipboard->Clear()`, comme l'Explorateur, pour ne pas déplacer deux fois.
- **Plusieurs fichiers** (venant de l'Explorateur) : coller chacun. L'arbre lui-même ne coupe ou copie qu'un élément.
- **Copier le chemin** : `wxTextDataObject`.
- Les fichiers collés peuvent venir de l'extérieur de la racine du projet.
- Si l'on coupe depuis l'arbre et colle dans l'Explorateur, c'est l'Explorateur qui déplace : les onglets ouverts ne suivent pas. Limite acceptée. L'arbre se met à jour à la réactivation du dialogue.
- Le format « Preferred DropEffect » n'a d'effet que sous Windows ; ailleurs il est ignoré, le code reste portable.

#### Coller : règles

- Cible : `GetTargetDir()`. Destination = cible + nom de la source.
- Couper puis coller dans le même répertoire : rien à faire.
- Destination existante (y compris une copie collée dans son propre répertoire) : `MakeUniqueName`.
- Répertoire collé dans lui-même ou un de ses sous-répertoires : refusé, message.
- Pas de confirmation.
- Ensuite `Rebuild` en sélectionnant le dernier élément collé.

#### Supprimer

Confirmation (`wxMessageBox`, `wxYES_NO | wxNO_DEFAULT`) construite avec `Describe` : fichier ou répertoire avec son nombre de fichiers, documents ouverts, modifications non enregistrées qui seront perdues. Réutiliser ou adapter les textes `AgentConfirmDelete...` sous de nouvelles clés (ils parlent de « l'agent »). Puis `FileOperations::Delete` (corbeille, onglets fermés), et `Rebuild` en sélectionnant le voisin suivant, sinon le précédent, sinon le parent.

### C. Git (dernière étape)

- **Détection** : remonter depuis le fichier jusqu'à un `.git` (répertoire, ou fichier pour les worktrees et sous-modules). Puis vérifier que la source est suivie : `git ls-files --error-unmatch -- <chemin>` (code de retour 0). Il faut `git` dans le PATH, sinon comportement sans git. Option `config.ini` pour désactiver (par exemple `file_operations_use_git=true` par défaut).
- **On ne touche à git que si la source est suivie.** Un fichier non suivi est traité sans git ; on n'ajoute jamais un fichier à git de notre propre initiative.
- **Déplacement** : `git mv -- <source> <destination>` à la place de `wxRenameFile`, exécuté depuis la racine du dépôt. Il renomme sur le disque et dans l'index en gardant le contenu indexé : les modifications non indexées restent non indexées (contrairement à « renommer puis `git add` », qui les indexerait). Validations, création du répertoire parent et documents qui suivent : identiques. Si `git mv` échoue sans avoir rien déplacé : renommage simple et avertissement que l'index n'a pas été mis à jour.
- **Suppression** : corbeille d'abord (comme sans git), puis `git rm --cached --quiet -- <chemin>` (`-r` pour un répertoire). L'index est le même qu'après `git rm`, et la corbeille reste un filet de sécurité. Si git échoue après la mise à la corbeille, ne pas annuler : avertir seulement.
- **Copie, création** : rien.
- **Exécution** : `wxExecute` synchrone avec `wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE`, répertoire de travail = racine du dépôt (`wxExecuteEnv::cwd`), sortie capturée pour les messages d'erreur.
- L'agent en profite automatiquement (`move_file`, `delete_file`). Mettre à jour `notes-agents.md` pour le signaler.

## Étapes

Compiler (`c.bat`) à la fin de chaque étape. L'utilisateur teste dans l'interface.

1. **`FileOperations`, sans changement de comportement.** Créer le module (déplacer, supprimer, `Describe`, `NormalizePath`, `FindOpenDocuments`, `CloseDocuments`, `IsValidName`). Réécrire `ToolMoveFile` et `ToolDeleteFile` comme adaptateurs : résolution des chemins et messages en anglais dans `AgentTools.cpp`, le reste dans le module. Supprimer les utilitaires devenus inutiles de `AgentTools.cpp`.
   *Fini quand* : l'agent déplace, renomme (y compris changement de casse seul), déplace un répertoire avec des fichiers ouverts, supprime un fichier ouvert non enregistré, exactement comme avant.
2. **`FileTree` sans nouvelle action.** Composant, `FileTreeItemData` sur fichiers et répertoires, activation directe, `Rebuild(pathToSelect)`, branchement dans le dialogue (renommé `FileTreeDialog`), rafraîchissement à la réactivation, `App::OnFilesChanged()` appelé par `FileOperations`.
   *Fini quand* : Ctrl+P se comporte comme avant (filtre, ouverture, OK/Annuler), et un fichier déplacé par l'agent apparaît à sa nouvelle place.
3. **Suppr, F2, menu contextuel.** Tester d'abord l'édition en place dans le dialogue (voir « Risques »).
   *Fini quand* : renommer un fichier ouvert met à jour son onglet ; un nom invalide relance l'édition ; supprimer un fichier ouvert ferme son onglet après confirmation ; le menu contextuel offre ces actions.
4. **Presse-papiers.** `Copy`, `MakeUniqueName`, couper/copier/coller, copier le chemin ; menu contextuel complété.
   *Fini quand* : copier dans l'arbre puis coller dans l'Explorateur fonctionne, et inversement ; couper dans l'Explorateur et coller dans l'arbre déplace le fichier ; coller une copie dans son propre répertoire donne `nom2.ext`, puis `nom3.ext`.
5. **Nouveau fichier, nouveau répertoire.** `CreateFile`, `CreateDir`, nœud temporaire, répertoires vides visibles sans filtre.
   *Fini quand* : Ctrl+N propose « nouveau.txt » en édition, Échap ne crée rien, Entrée crée le fichier et le sélectionne ; idem pour Ctrl+Shift+N avec « Nouveau dossier », qui reste visible bien que vide.
6. **Git.** Détection, `git mv`, `git rm --cached`, option, mise à jour de `notes-agents.md`.
   *Fini quand* : dans un dépôt, renommer un fichier suivi montre « renamed » dans `git status`, supprimer montre « deleted » indexé et le fichier est dans la corbeille ; un fichier non suivi est traité sans git.

## Risques à vérifier tôt

- **Édition en place dans un dialogue** : Entrée pendant l'édition pourrait déclencher le bouton par défaut (OK) et Échap masquer le dialogue (`OnCancel`). wxWidgets gère normalement ces cas pour le contrôle d'édition du TreeView sous MSW, mais c'est le premier point à tester à l'étape 3. Si besoin, filtrer dans `wxEVT_CHAR_HOOK` du dialogue quand `GetEditControl()` n'est pas nul.
- **Raccourcis** : F2, Ctrl+N... ne doivent pas atteindre la fenêtre principale (le dialogue a `wxDIALOG_NO_PARENT`, normalement pas de propagation des accélérateurs).
- **Reconstruction pendant un événement de l'arbre** : toujours différer `Rebuild` avec `CallAfter` dans les gestionnaires d'événements de l'arbre.
- **Copie d'un gros répertoire** : synchrone, interface bloquée pendant la copie. Acceptable pour commencer.

## Avancement

- [ ] Étape 1 : `FileOperations`, outils d'agent refondus
- [ ] Étape 2 : `FileTree` branché dans le dialogue
- [ ] Étape 3 : supprimer, renommer, menu contextuel
- [ ] Étape 4 : presse-papiers
- [ ] Étape 5 : nouveau fichier, nouveau répertoire
- [ ] Étape 6 : git

## Pour plus tard (hors périmètre)

- Glisser-déposer dans l'arbre.
- Confirmation de suppression configurable ; vrai `git rm` configurable.
- **Arbre F6 (`TreeMarkerJumpList`)** : renommer, supprimer, monter/descendre des sections, traduits en modifications dans l'éditeur. Difficulté principale : `TextMarker` (`src/text/TextMarkerFinderInterface.hpp`) ne donne que la position du nom du marqueur (`start`/`end`, ou seulement `line`), pas la fin de la section. La règle « jusqu'au marqueur suivant de niveau inférieur ou égal » est exacte en Markdown, mais fausse dans des cas courants pour le code (commentaires de documentation, accolades fermantes) et fragile pour JSON/YAML. Piste : un champ `sectionEnd` rempli par les finders qui le connaissent, en commençant par Markdown seul ; des données structurées (indice du marqueur) à la place de la chaîne de saut rapide dans les nœuds.
