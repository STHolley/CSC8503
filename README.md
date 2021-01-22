# CSC8503

# Menu controls:
Main menu:
  Esc: Close window / game
  P: Start in single player
  M: Start in multiplayer (AI)
  
In Game:
  W: Move forward
  S: Move backwards
  A: Turn left
  D: Turn right
  SPACE: Jump
  ESCAPE: Pause
  F1: Return to main menu
  F2: Toggle freecam
  
Pause Menu:
  ESCAPE: Unpause
  
Game Over Screen:
  F1: Return to main menu
  
# About
# AI
AI uses A* grid based path finding to navigate around the maze. AI only shows up when running in multiplayer mode.
The AI will return to the starting zone if it ends up falling off the play area. (similar to the player)
Both players and AI can collect coins. AI use a point system like the player which can run out. Coins can only be collected once per game for all players.
If an enemy reaches the end before you, you will lose. However, your score will still be displayed on the game over screen.

# World
There is a 'Rope Bridge' in the centre of the map with pitfalls either side.
At the start there is a moving barrier which will slow or prevent movement until it has moved away.

The finish goal is the furthest point from the starting zone. Although unmarked it should feel apparent.

# Physics
Most objects use axis aligned bounding boxes for collisions, with coins using spherical bounding volumes.

All player / AI movement is force based with rotations. Rotation affects resultant force direction.
To keep things feeling semi-realistic, the players and floor have 0 elasticity so collisions should feel rigid. Walls on the other hand are bouncy giving the player a push away when running into them.
