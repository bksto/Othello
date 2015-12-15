# Othello
High Level Description:

Othello is a two-player game where each opponent places his or her piece (either blue or green) piece onto the 6x6 game board. The player is able to traverse through the LED matrix with the up, down, left, right buttons connected to the ATmega1284P, also detecting if there is a valid move. When the piece is placed on the board, the board checks if there are possible pieces to flip by checking if the opponent’s piece is in between the player’s pieces.  When the board is full (by adding the player’s score together), and the winner is determined by comparing the player’s scores, which determines which color to display at the winner’s state.  

User Guide:

The goal of Othello is to fill the board with your color. To do so, the current player must place their piece next to the opponents piece and trap the opponent’s pieces in between the their pieces, either diagonally, up, down, left and right. This will change the opponent’s color to the current player’s color. So the player with the most color on the board in the end of the game wins. Press RESET to restart the game. 
The player may use the outermost square to maneuver around the board but cannot place their piece there. 

Technologies and components used:
-	AVR Studio 6
-	ATmega1284P microprocessor 
-	ATmega1284 microprocessor
-	Breadboard 
-	RioRand(TM) LED Matrix 8x8
-	16 x 2 LCD Screen 
-	Push buttons
-	330 Ohms resistor (8)
-	Battery holder

Demo Video: 
	http://youtu.be/vsCDrTnYWWY
