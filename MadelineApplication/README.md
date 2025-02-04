------------------
Keyboard Controls:
ARROW KEYS - Movement / aiming dash
C - Jump / Confirm
X - Dash / Back
Z + UP and DOWN - Climb

ESC - Pause
Z - credits on main menu
DELETE - reset save data on main menu
TAB - view colliders

Level Editor:
    Notes: - The editor by default will only show the current scene you are in and the next one. To make other
             scenes appear for editing, with the editor open click inside the boundries of another scene to make
             it appear. You can then place new tiles.  
           - Known Bugs: - The level editor will not show the mouse cursor or the current tile you have selected 
                           when editing. To fix this simply close and reopen the application and it should display correctly.
                         - The Spawn Point tiles are not working as intended, they currently save the position where you click 
                           to activate them in the previous scene as the spawn point instead the intended spawn point

L - Enter / Exit Editor - Changes are saved on editor exit, NOT on application close
    LMB - Place a tile at mouse position
    RMB - Remove a tile at mouse position
    MIDDLE MOUSE BUTTON - Pan Camera
    SCROLL - Zoom In / Zoom Out

0-9 - Select Tile to Place
    1 - Gray Tile
    2 - Orange Tile
    3 - Blue Tile
    4 - Green Tile
    5 - Jump-Pad - Player bounces when landed on
    6 - Scene Exit - Must be placed on the border between 2 scenes to allow traversal between scenes
    7 - Spawnpoint(Not Working) - Spawnpoints are selected by what scene you entered from, after placing a spawnpoint
        click inside of a another scene to select which scene the player enters from to activate it (This feature is currently not working correctly)
    8 - Dash Crystal - If player dashes while touching this item, they are allowed to dash again before 
        touching the ground
    9 - Strawberry - A collectible item placed in challenging locations, player must overlap, then touch 
        the ground without dying to collect
    0 - Tombstone - When player touches this they complete the level and go back to the menu
    LSHIFT + 1 - Spikes - Player dies if they touch this tile
    LSHIFT + 2 - One-way platform - Player can jump through from the bottom but not vice versa
    LSHIFT + 3 - Falling platform - Tile will fall after player lands on it

N - Enter / Exit New Scene Editor
    Notes: - Only one scene can be placed per editing session, after creating new scene, close 
             New Scene Editor and Level Editor, then reopen both to place an additional scene
           - Must place first scene boundry in the top left corner of the desired scene, followed by second 
             boundry in the bottom right corner of the desired scene, the other two corners will be added 
             once you do this
           - Must manually delete scene files if you want to delete scenes, go to the following path
             "../MadelineApplication/Scenes/", scenes 1-5 will be there by default and any additional added
             scenes will be numbered sequentially
           - Do not rename scene files or they will not work as they are read in by file names in this format "Scene(x)"

    LMB - Place a new scene boundary corner
    RMB - Remove first scene boundary corner to replot(if you place the second boundry you will not be able to use RMB to 
          delete, must manually delete file to remove as specified above)

-----------------
Gamepad Controls:
LEFT STICK - Movement / aiming dash
SOUTH BUTTON - Jump / Confirm
WEST BUTTON - Dash / Back
LEFT TRIGGER + LEFT STICK - Climb

START BUTTON - pause
BUTTON EAST - back
BUTTON NORTH - credits on main menu
SELECT BUTTON - reset save data on main menu