# StreamDiscord — prototype Windows

Capture une fenêtre et rejoue le son d'une application indépendante dans le processus StreamDiscord, afin de partager sa fenêtre de diffusion dans Discord.

## Lancer

Ouvrir `build/StreamDiscord.exe`. L'interface utilise Dear ImGui, intégré à l'exécutable : aucune installation supplémentaire n'est nécessaire.

1. Dans **Vidéo**, choisir **Profil TFT**, saisir le nom d'un exécutable (par exemple `League of Legends.exe`) ou sélectionner une fenêtre ouverte.
2. Dans **Audio**, laisser le champ vide pour suivre le processus vidéo, ou saisir `chrome.exe` pour relayer Chrome. La liste des fenêtres permet aussi de sélectionner une autre application.
3. Dans **Sortie audio**, choisir une sortie non écoutée, par exemple un écran dont les haut-parleurs sont coupés, ou un câble audio virtuel. Sans sélection, le relais audio est désactivé et Discord ne reçoit pas de son de l'application. Cliquer sur **Démarrer la capture**, même si le jeu n'est pas lancé.
4. L'application vérifie toutes les 750 ms si une fenêtre correspondante apparaît. Si elle disparaît, elle repasse en attente et se reconnecte à sa réapparition.
5. Partager **StreamDiscord — Diffusion** dans Discord avec le son activé.

Les noms des sources sont sauvegardés dans `sources.ini` à côté de l'exécutable au démarrage de la capture. Après relancement du logiciel, cliquer sur **Démarrer la capture** pour armer le profil. Les champs Vidéo et Audio sont verrouillés pendant la capture : arrêter pour les modifier. La sortie audio peut être changée pendant la capture.

## Limites à tester

- Le profil TFT cible la fenêtre de partie `League of Legends.exe`, pas le client Riot/League. Le nom devra être adapté si l'installation utilise un autre exécutable.
- La capture TFT et la récupération du son par Discord doivent encore être validées en situation réelle. Privilégier le mode fenêtré sans bordure pour le premier essai.
- Chrome est capturé par arbre de processus : les autres onglets audibles peuvent être inclus. Pas d'isolation d'un seul onglet à ce stade.
- Le son est rejoué sur la sortie explicitement sélectionnée. Garder le son original dans le casque et envoyer le relais sur une autre sortie non écoutée évite le doublon local. Choisir le casque comme sortie du relais reproduit le doublon. Le routage audio via une autre sortie doit encore être validé côté Discord.
- Si plusieurs fenêtres du même exécutable sont présentes, la première fenêtre admissible est choisie. Les fenêtres masquées, sans titre ou possédées par une autre fenêtre sont ignorées.
- La vidéo est étirée aux dimensions de la fenêtre de diffusion ; le respect du ratio reste à améliorer. Certaines fenêtres minimisées ou protégées peuvent ne pas fournir d'image.
- Une erreur audio est affichée avec son code ; utiliser Arrêter / Démarrer pour réessayer. Les changements de périphérique audio en cours de capture ne sont pas encore gérés.

## Compiler

Visual Studio Build Tools 2022, MSVC v143 et SDK Windows 10.0.22000.0. Depuis PowerShell : `./build.ps1`.

Application native C++17, Win32, Windows Graphics Capture / D3D11 et WASAPI Process Loopback. Interface Dear ImGui v1.91.9b avec les backends officiels Win32 et DirectX 11. Liaison statique du runtime C++. Sources et licence MIT incluses dans `third_party/dear_imgui` ; version exacte dans `third_party/README.md`.

## Organisation du code

| Module | Responsabilité |
| --- | --- |
| `main.cpp` | Point d'entrée Windows |
| `application.h` / `application.cpp` | État de l'application, cycle de vie et coordination des captures |
| `application_ui.cpp` | Cartes, champs et actions de l'interface |
| `video_capture.h` / `video_capture.cpp` | Capture de fenêtre et rendu de la diffusion |
| `audio_capture.h` / `audio_capture.cpp` | Capture audio par processus et relais WASAPI |
| `audio_devices.h` / `audio_devices.cpp` | Énumération des sorties audio |
| `interface.h` / `interface.cpp` | Rendu DirectX 11 et thème Dear ImGui |
| `window_sources.h` / `window_sources.cpp` | Énumération des fenêtres et des exécutables |
| `settings.h` / `settings.cpp` | Lecture et écriture du profil INI |
| `text.h` / `text.cpp` | Conversions UTF-8 / UTF-16 |

Les en-têtes exposent les déclarations ; les implémentations résident dans les fichiers `.cpp`. L'état est encapsulé dans `Application`. Les sources tierces restent séparées dans `third_party`.

Le style est défini dans `.clang-format` et `.editorconfig` : indentation de quatre espaces, blocs avec accolades, lignes limitées à 100 colonnes. `./format.ps1` applique le formatage aux sources du projet, `./format.ps1 -Check` le vérifie sans modifier les fichiers. Le script utilise `clang-format` du PATH ou celui fourni par Visual Studio ; les fichiers Dear ImGui ne sont pas reformattés.

Après le découpage en modules : compilation Release x64 réussie, vérification du formatage passée, puis contrôle visuel du lancement, de la capture du client League, du masquage/réaffichage de la diffusion et de l'arrêt. Le relais Chrome s'initialise sans erreur ; la transmission finale à Discord conserve les limites de validation décrites ci-dessous.

## Vérifications effectuées

- Compilation Release x64 réussie sur la machine de développement.
- Interface ouverte et mode d'attente TFT vérifié.
- Détection d'une application lancée après armement : Paint capturé automatiquement ; image contrôlée dans la fenêtre de diffusion.
- Source audio laissée vide : le statut suit Paint et le client audio s'initialise sans erreur. Ce test silencieux ne valide pas la qualité audio ni la transmission à Discord.
- Profil TFT restauré et sauvegardé après le test.

## Références API

- https://learn.microsoft.com/en-us/windows/apps/develop/media-authoring-processing/screen-capture
- https://learn.microsoft.com/en-us/samples/microsoft/windows-classic-samples/applicationloopbackaudio-sample/

## Fenêtre de diffusion et sortie audio

La croix de la diffusion masque la fenêtre sans la détruire ni arrêter la capture. **Ouvrir la diffusion** la rouvre ; démarrer une capture la réaffiche également. Fermer la fenêtre principale quitte toujours le logiciel. La fenêtre de diffusion reste masquée au lancement jusqu'à son ouverture ou au démarrage d'une capture.

Le sélecteur des sorties liste les périphériques de lecture actifs et marque la sortie Windows par défaut. Le choix est sauvegardé. Si la sortie enregistrée est absente au lancement ou après actualisation, le relais est désactivé, sans basculer vers le casque. Une sortie retirée pendant le relais affiche une erreur : actualiser les sorties puis sélectionner une sortie disponible. **Relais désactivé** coupe le relais de l'application, donc aussi son audio pour Discord ; ce n'est pas un mode silencieux local avec audio Discord conservé.

Validation de cette version : compilation Release x64, liste des sorties réelles, fermeture/masquage puis réouverture de la même fenêtre vérifiés. L'audibilité sur la sortie dédiée et la récupération de cette sortie par Discord restent à tester.

## Interface 0.2

Thème sombre, cartes Vidéo / Audio, section Sortie audio, état de capture et actions regroupées. Textes neutres, navigation au clavier via Tab et prise en charge des accents et de la ponctuation française. La mise en page suit la largeur de la fenêtre ; une barre de défilement permet l'accès aux actions si la hauteur est insuffisante. Dear ImGui n'expose pas les widgets dans l'arbre UI Automation Windows.

Vérifications : compilation avec les sources vendues de Dear ImGui, inspection à taille normale et maximisée, menu de sortie audio, démarrage/arrêt de capture du client League, image contrôlée dans la fenêtre de diffusion, initialisation du relais Chrome sans erreur. Les réglages existants ont été conservés. La transmission audio à Discord reste à valider en conditions réelles.

