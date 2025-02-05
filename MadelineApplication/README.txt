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

Notes: - The editor will load with only the scene you are in and the next scene rendered, as well as all current scene boundries. 
	 To edit other scenes click within the boundries of the unrendered scene to make it appear for editing. 
         
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
    7 - Spawnpoint(Not Working) - Spawnpoints are selected by what scene you entered from, after placing a 
	spawnpoint click inside of a another scene to select which scene the player enters from to activate it
        (This tile is not working as intended and instead is saving the place of the click inside the previous scene)
    8 - Dash Crystal - If player dashes while touching this item, they are allowed to dash again before 
        touching the ground
    9 - Strawberry - A collectible item placed in challenging locations, player must overlap then touch 
        the ground without dying to collect
    0 - Tombstone - When player touches this they complete the level and go back to the menu
    LSHIFT + 1 - Spikes - Player dies if they touch this tile
    LSHIFT + 2 - One-way platform - Player can jump through from the bottom but not vice versa
    LSHIFT + 3 - Falling platform - Tile will fall after player lands on it



New Scene Editor:

Notes: - Only one scene can be placed per editing session. To create additional scenes you must close the
	 new scene editor and the level editor, then reopen both.
       - To delete scenes, you must manually delete the file. Go to the following path 
	 "..\DirectX11-Madeline-main\DirectX11-Madeline-main\MadelineApplication\Scenes"
       - The game will come with scenes 0-5 loaded by default and all additional scenes will be numbered sequentially.
       - Do not rename scenes as they will not load. They must be in the following format "Scene(x)"

N - Enter / Exit New Scene Editor

    LMB - Place a new scene boundary corner
    RMB - Remove first scene boundary corner to replot (Will not delete any corners after second corner is placed)



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