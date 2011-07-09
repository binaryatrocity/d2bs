//	Save this script in your /tools/ folder 

//	Load this script by adding the following line to your 
//	runner's char config file (anywhere on a line by itself: (take out the comment though)
//	
//	load('tools/announceGame.js');

function main(){
	announceGame();
}

function announceGame(){
	var gameInfo = ''+ me.realm +' '+ me.core +' '+ me.ladder +' '+ me.gamename +' '+ me.gamepassword;
	sendDDE(1, 'mIRC', 'COMMAND', '', '/announceGame '+ gameInfo);
};



  
/*	mIRC Script for Remote Scripts section ;; Add this Alias to your Remote scripts in mIRC (Alt+R) and change the %announceChan var.


Alias announceGame {

  ;; Format: /announceGame realm core gameType charName charLvl charClass gameDiff gameName gamePassword

  ;; Workaround for NTBot spamming the sendDDE() func from the script when in char config file..
  ;; (only allows mIRC to announceGame once every 15 seconds)
  
  var %announceChan = "#IRCCHANNELHERE"

  if ($var(gameAnnounceActive,0) > 0) { return }
  else { set -u15 %gameAnnounceActive true }

  if ($me ison %announceChan) {
    var %gameRealm = $1
    var %gameCore = $2
    var %gameType = $3
    var %charName = $4
    var %charLvl = $5
    var %charClass = $6
    var %gameDiff = $7
    var %gameName = $8
    var %gamePassword = $9

    if ( baal isin $lower($gameName)) { var %runType = new BAAL }
    else if ( chaos isin $lower($gameName)) { var %runType = new CHAOS }
    else { var %runType = new }

    msg %announceChan (10 $+ %gameRealm %gameCore %gameType $+ 1) 7 $+ %charName $+ , my level %charLvl %charClass has started a %runType game:4  %gameName 1//4 %gamePassword
  }

}


*/