# UVGroupUtil
UV Group Utilities is a plug-in for Maya designed to extend the functionality of Maya's UV editor to list and group UV shells.

## Installation
1. Copy `UVGroupUtil.mll` from the downloaded folder to the Maya plug-ins directory:
- Windows: `%homepath%\Documents\Maya\plug-ins` (or `C:\users\<username>\Maya\plug-ins`)
> If the `plug-ins` directory does not exist within the Maya folder, create it manually

2. Launch Maya and open the plug-in manager by selecting `Windows > Settings/Preferences > Plug-in Manager`
*It is recommended that this is done in a new, empty scene, but not required*

3. Locate `UVGroupUtil.mll` listed under the appropriate path and select Loaded

4. Select Accept in the pop-up dialog if prompted

5. Auto-load may now also be checked if you would like the plug-in to load with Maya on each startup

  
## Features

### Groups
Containers that other groups or UV shells can be assigned to
- Automatically serialize on save and deserialize on load

### UV Outliner
UV Outliner window designed to mirror Maya's built-in outliner that lists the UV hierarchy for each mesh in the scene and acts as a visualizer for groups
- Interactive updates for mesh creations and deletions[^1]
- Interactive updates for UV shell splits, merges, creations and deletions with respect for existing groups
- UV shell grouping
- Recursive UV shell grouping (groups containing groups)
- Group deletion with automatic hierarchy updates
- Reparenting groups and shells by dragging and dropping in the UV outliner

### Layout commands

[^1]: Transient meshes (e.g. when duplicating) are not  yet supported

## Planned features

### UV Outliner
- Support for hidden objects
- Support for multiple layers
- Quick access option to delete all groups
- Support for object reordering

### Layout commands
- Optimization for large scenes
- More reliable recursive layout
- Freezing/locking groups
- Complex bounding shapes for more efficient layouts

### Visual overlay in the UV editor

### Misc
- Undo