#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <time.h>
#include <netdb.h>
#include <stdbool.h>

#define MAX_CONNECTIONS 10000
#define INITIALCOINS 600


struct Player
{
	char* name ;
	int id ;
	int coins ;
	int level ;
	int score ;
	int clientfds ;
	bool isActive ;
	bool win ;
} ;

struct gamesession 
{
	bool isActive ;
	int numberOfPlayers ;

	struct Player** players ;
} ;

struct Pair
{
	int level ;
	int index ;
} ;

typedef struct gamesession GameSession ;
typedef struct Player Player ;
typedef struct Pair Pair ;

GameSession **games ;
int count = 0 ;
bool cardsGiven[3][52] ;

// error - print message and exit with code -1
void error(const char *msg) 
{
	printf("%s\n", msg);
	fflush(stdout) ;
	// exit(-1);
}

Player* createPlayer(int clientfds, char* name, int id)
{
	Player* newPlayer = (Player*)calloc(1, sizeof(Player)) ;
	newPlayer->coins = INITIALCOINS ;
	newPlayer->level = 0 ;
	newPlayer->win = false ;
	newPlayer->clientfds = clientfds ;
	int i ;
	char* temp = (char*)calloc(20, sizeof(char)) ;
	for(i = 0; i < 18; ++i)
		temp[i] = name[i] ;
	newPlayer->name = temp ;
	newPlayer->id = id ;
	newPlayer->score = 0 ;
	++count ;

	return newPlayer ;
}

void addPlayerToLevel(Player* newPlayer, int level)
{
	int i = 0 ;
	games[level]->numberOfPlayers = games[level]->numberOfPlayers + 1 ;
	// if(level == 1)
	// {
	// 	printf("The Number of Players on level 2 are : %d\n", games[level]->numberOfPlayers);
	// }
	for(i = 0; i < 10; ++i)
	{
		if(games[level]->players[i] == NULL)
		{
			games[level]->players[i] = newPlayer ;
			return ;
		}
	}

	return ;
}

void removePlayerFromLevel(Player* player, int level)
{
	int id = player->id ;
	int i = 0 ;

	for(i = 0; i < 10; ++i)
	{
		if(games[level]->players[i] != NULL && games[level]->players[i]->id == id)
		{
			games[level]->players[i] = NULL ;
			return ;
		}
	}

	// error("Player Not Found\n") ;

	return ;
}

Pair findPlayer(int id)
{
	Pair answer ;
	answer.level = 0 ;
	answer.index = 0 ;
	int i, j ;
	for(i = 0; i < 3; ++i)
	{
		for(j = 0; j < 10; ++j)
		{
			if(games[i]->players[j] != NULL && games[i]->players[j]->id == id)
			{
				answer.level = i ;
				answer.index = j ;
				return answer ;
			}
		}
	}

	// error("Player Not Found\n") ;

	return answer ;
}

Player* getPlayer(int id)
{
	Pair answer ;
	answer.level = 0 ;
	answer.index = 0 ;
	int i, j ;
	for(i = 0; i < 3; ++i)
	{
		for(j = 0; j < 10; ++j)
		{
			if(games[i]->players[j] != NULL && games[i]->players[j]->id == id)
			{
				return games[i]->players[j] ;
			}
		}
	}

	// error("Player Not Found\n") ;

	return NULL ;
}

void resetCardsGiven(int level)
{
	int i = 0 ;
	for(i = 0; i < 52; ++i)
		cardsGiven[level][i] = false ;

	return ;
}

int getCard(int level)
{
	int i = 0 ;
	int val = rand()%52 ;
	while(cardsGiven[level][val])
		val = rand()%52 ;
	cardsGiven[level][val] = true ;

	return val ;
}

void giveCards(int level)
{
	int i = 0; 
	resetCardsGiven(level) ;
	games[level]->isActive = true ;
	for(i = 0; i < 10; ++i)
	{
		if(games[level]->players[i] != NULL)
		{
			int cardVal = getCard(level) ;
			games[level]->players[i]->score = (cardVal%14) + 1 ;
			games[level]->players[i]->isActive = false ;
		}
	}

	return ;
}

void evaluateCoins(int level)
{
	int i = 0 ;
	int maxScore = 0 ;
	int count = 0;
	int countWithMaxScore = 0 ;

	for(i = 0; i < 10; ++i)
	{
		if(games[level]->players[i] != NULL && games[level]->players[i]->isActive == true)
		{
			++count ;
			if(maxScore < games[level]->players[i]->score)
			{
				maxScore = games[level]->players[i]->score ;   
				countWithMaxScore = 1 ;
			}
			else if(maxScore == games[level]->players[i]->score)
			{
				++countWithMaxScore ;
			}
		}
	}
	int coinsBetted ;
	if(level == 0)
		coinsBetted = 50 ;
	else if(level == 1)
		coinsBetted = 100 ;
	else if(level == 2)
		coinsBetted = 150 ;
	else
		error("Level out of bounds in evaluateCoins") ;
	if(countWithMaxScore == 0)
		return ;
	int increaseInCoins = (count*coinsBetted)/countWithMaxScore ;
	for(i = 0; i < 10; ++i)
	{
		if(games[level]->players[i] != NULL && games[level]->players[i]->isActive == true)
		{
			++count ;
			// printf("%s HAHAHAH %d\n", games[level]->players[i]->name ,games[level]->players[i]->score);
			games[level]->players[i]->coins = games[level]->players[i]->coins - coinsBetted ;
			if(maxScore == games[level]->players[i]->score)
			{

				games[level]->players[i]->coins = games[level]->players[i]->coins + increaseInCoins ;
				games[level]->players[i]->win = true ;
			}
		}
		if(games[level]->players[i] != NULL)
		{
			games[level]->players[i]->isActive = false ;
			games[level]->players[i]->score = 0 ;
		}
	}

	return ;
}

// function get_serverfd()
// takes a port and returns a socket file descriptor, that has already succesfully called bind()
// if there is an error in binding, the program exits.
// supports both IPv4 and IPv6, and uses TCP sockets
int get_serverfd(char *port) {
  int fd, s;

  // used for getaddrinfo() to support both IPv4 and IPv6
  struct addrinfo hints;
  struct addrinfo *result, *rp; 
 
  // clear the fields of hints
  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(NULL, port, &hints, &result);

  // check for error
  if (s != 0) 
	error("error getting address info\n");

  // try each address until we successfully bind
  for( rp=result; rp != NULL; rp = rp->ai_next) {
	fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if (fd < 0)
	  continue;
	if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
	  break;  // success
	close(fd);
  }
  
  // no address succeeded
  if(rp == NULL) 
	error("Could not bind\n");
  freeaddrinfo(result);

  // bind successful, return the fd
  return fd;
}


// main function
int main(int argc, char **argv) 
{
  srand(time(NULL));

  int serverfd;  // socket fd for server
  int clientfds[MAX_CONNECTIONS]; // an array sockets fds for clients
	games = (GameSession **) malloc( 3 * sizeof(GameSession *));
	int k ;
	int maxfd;
	for (k = 0; k < 3; k++) 
	{
	    games[k] = (GameSession *)calloc(1, sizeof(GameSession *)) ;
	    games[k]->isActive = false ;
	    games[k]->players = (Player **)calloc(10, sizeof(Player *)) ;
	}
	int gameNum = 0;
	// int playersInQueue[MAX_CONNECTIONS];
	int numPlayers = 0;

	int i;
	// for (i = 0; i < MAX_CONNECTIONS; i++)
	//     playersInQueue[i] = -1;

  for( i=0; i< MAX_CONNECTIONS; i++)
	clientfds[i] = -1;

  int ncc = 0;    // number of client connections
  char buffer[1024];
  // client addr structure
  struct sockaddr_in cliaddr;
  socklen_t clilen;
  clilen = sizeof(cliaddr);

  // read fd structure used for select
  fd_set readfds;

  // program requires 1 argument - the port the server will listen on
  if (argc != 2) {
	error("Usage: server portNumber\n");
  }

  // get the server socket fd, and mark it as a passive socket
  serverfd = get_serverfd(argv[1]);
  if ( listen(serverfd, MAX_CONNECTIONS) < 0 )
	error("listen error\n");
  maxfd = serverfd + 1;
 
  int check[MAX_CONNECTIONS];

  int z;
  for(z = 0; z < MAX_CONNECTIONS; z++) 
  {
	check[z] = 0;
  } 


  // infinite loop
  while(1) {
	FD_ZERO(&readfds);
	FD_SET(serverfd, &readfds);
	int j;
	for (j = 0; j < ncc; j++) {
	  if(clientfds[j] >= 0)
		FD_SET(clientfds[j], &readfds);
	}
	
	int res = select(maxfd, &readfds, NULL, NULL, NULL);
	if ((res < 0)) 
	{
	  error("select error");
	}

	if (FD_ISSET(serverfd, &readfds) ) {
	  clientfds[ncc] = accept(serverfd, (struct sockaddr *) &cliaddr, &clilen);
	  printf("client fd %d connected\n", clientfds[ncc]);
	  if(clientfds[ncc] > serverfd)
		maxfd = clientfds[ncc] + 1;
	  ncc++;
	}
	else {
	  // check if client has received a packet
	  int j=0;

	  bool isCardsGiven = false ;
	  int roundCount = 1 ;
	 // for(j = 0; j < ncc; j++) {
		while(1){
		if(j >= ncc)
		{
			++roundCount ;
		}
		j = j % ncc;

		if(j == 0)
		{
			resetCardsGiven(0) ;
			resetCardsGiven(1) ;
			resetCardsGiven(2) ;
			if(roundCount%2 == 1)
			{
				evaluateCoins(0) ;
				evaluateCoins(1) ;
				evaluateCoins(2) ;
				int i ;
				int level ;
				for(level = 0; level < 3; ++level)
				{
					for(i = 0; i < 10; ++i)
					{
						if(games[level]->players[i] != NULL)
						{
							int coins = games[level]->players[i]->coins ;
							if(coins < 850)
							{
								if(level == 1)
								{
									games[1]->numberOfPlayers = games[1]->numberOfPlayers - 1 ;
									int tempId = games[level]->players[i]->id ;
									Player* tempPlayer = games[level]->players[i] ;
									games[level]->players[i]->level = 0 ;
									games[level]->players[i] = NULL ;
									addPlayerToLevel(tempPlayer, 0) ;
								}
							}
							else if(coins >= 850 && coins < 1100)
							{
								if(level == 0)
								{
									games[0]->numberOfPlayers = games[0]->numberOfPlayers - 1 ;
									int tempId = games[level]->players[i]->id ;
									Player* tempPlayer = games[level]->players[i] ;
									games[level]->players[i]->level = 1 ;
									games[level]->players[i] = NULL ;
									addPlayerToLevel(tempPlayer, 1) ;
								}
								else if(level == 2)
								{
									games[2]->numberOfPlayers = games[2]->numberOfPlayers - 1 ;
									Player* tempPlayer = games[level]->players[i] ;
									games[level]->players[i]->level = 1 ;
									games[level]->players[i] = NULL ;
									addPlayerToLevel(tempPlayer, 1) ;
								}
							}
							else if(coins >= 1100)
							{
								if(level == 1)
								{
									games[1]->numberOfPlayers = games[1]->numberOfPlayers - 1 ;
									Player* tempPlayer = games[level]->players[i] ;
									games[level]->players[i]->level = 2 ;
									games[level]->players[i] = NULL ;
									addPlayerToLevel(tempPlayer, 2) ;
								}
							}
						}
					}
				}
				for(level = 0; level < 3; ++level)
				{
					for(i = 0; i < 10; ++i)
					{
						if(games[level]->players[i] != NULL)
							printf("%s at level : %d has %d coins.\n", games[level]->players[i]->name, level+1, games[level]->players[i]->coins) ;
					}
				}

			}
		}
		Pair PlayerThisTurn = findPlayer(j) ;
		if(games[PlayerThisTurn.level]->numberOfPlayers < 3 && roundCount > 1)
		{

			FD_SET(clientfds[j+1], &readfds);
			j++;
			continue ;
		}
		  
		if (clientfds[j] < 0)
		  continue;

		// printf("%d %d %d %d\n", FD_ISSET(clientfds[j], &readfds ), j, check[0], check[1]);

		if ( FD_ISSET(clientfds[j], &readfds )) {
			int trial =0;

			int h =0;
			for(h=0;h< ncc;h++)
			{
			  if(check[h] == 1)
				trial =1;
			  else
			  {
				trial =0;
				break;
			  }  
			}

			if(trial == 1)
		   {
			   if (send(clientfds[j], "turn", 256, 0)< 0)
					  error("Error sending turn");

			  while(recv(clientfds[j], buffer,1024, 0) <= 0)
				continue;
			if(roundCount%2 == 1)
			{
			  Pair levelAndIndex = findPlayer(j) ;
			  int index = levelAndIndex.index ;
			  int level = levelAndIndex.level ;
			  int cardVal = getCard(level) ;
			  games[level]->players[index]->score = (cardVal%13)+1 ;
			  games[level]->players[index]->isActive = false ;
			  char setcard[7] ;
			  setcard[0] = (cardVal/10) + '0' ;
			  setcard[1] = (cardVal%10) + '0' ;
			  
               printf("%s got : %d of ", games[level]->players[index]->name, cardVal%13+1);
               if(cardVal/13 == 0)
                  printf("Spades\n");
               else if(cardVal/13 == 1)
                  printf("Clubs\n");
               else if(cardVal/13 == 2)
                  printf("Hearts\n");
               else if(cardVal/13 == 3)
                  printf("Diamonds\n");
              setcard[2] = (games[level]->players[index]->win ? '1' : '0') ;

				int coinsTemp = games[level]->players[index]->coins ;
				setcard[3] = coinsTemp/1000 + '0' ;
				coinsTemp %= 1000 ;
				setcard[4] = coinsTemp/100 + '0' ;
				coinsTemp %= 100 ;
				setcard[5] = coinsTemp/10 + '0' ;
				coinsTemp %= 10 ;
				setcard[6] = coinsTemp + '0' ;
			  setcard[7] = '\0' ;
			  if (send(clientfds[j], setcard, 7, 0)< 0)
				   printf("Error sending message");
				games[level]->players[index]->win = false ;
			}
			else
			{
				Pair levelAndIndex = findPlayer(j) ;
				int index = levelAndIndex.index ;
				int level = levelAndIndex.level ;
				fflush(stdout) ;
				if(strcmp(buffer, "1") == 0)
				{
					printf("%s has bet %d\n", games[level]->players[index]->name, (50)*(games[level]->players[index]->level + 1)) ;
					games[level]->players[index]->isActive = true ;
				}
				else if(strcmp(buffer, "0") == 0)
				{
					printf("%s didn't bet\n", games[level]->players[index]->name) ;
				}
				else
				{
					printf("Wrong Option\n") ;
				}
				char* name = games[level]->players[index]->name ;
				int indexTemp ;
				buffer[1] = games[level]->players[index]->level + '0';
				for(indexTemp = 2; name[indexTemp-2] != '\0'; ++indexTemp)
				{
					buffer[indexTemp] = name[indexTemp-2] ;
				}
				int u =0;
				int coinsTemp = games[level]->players[index]->coins ;
				buffer[100] = coinsTemp/1000 + '0' ;
				coinsTemp %= 1000 ;
				buffer[101] = coinsTemp/100 + '0' ;
				coinsTemp %= 100 ;
				buffer[102] = coinsTemp/10 + '0' ;
				coinsTemp %= 10 ;
				buffer[103] = coinsTemp + '0' ;
				for(u =0; u < ncc; u++)
				{
					Pair levelAndIndexTemp = findPlayer(u) ;
					int indexTemp = levelAndIndexTemp.index ;
					int levelTemp = levelAndIndexTemp.level ;

					if(level == levelTemp)
					{
				  		buffer[123] = '\0' ;
				  		send(clientfds[u], buffer, 256, 0);
					}
				  	else
				  	{
				  		buffer[123] = '0' ;
				  		send(clientfds[u], buffer, 256, 0);
				  	}
				}
				buffer[100] = buffer[101] = buffer[102] = buffer[103] = '\0' ;
			}
		   } 
		   else 
		   { 
				if (recv(clientfds[j], buffer,1024, 0) <= 0 ) {

	   
				 // clientfds[j] = -1;
				}
				// check if this is a want game request
				else if (buffer[255] == '0') {
					Player* newPlayer = createPlayer(clientfds[j], buffer, j) ;    
					addPlayerToLevel(newPlayer, 0) ;
				  printf("received a want game request from client %d\n", clientfds[j]);
					  int cardVal = getCard(0) ;
					  Pair levelAndIndex = findPlayer(j) ;
					  int index = levelAndIndex.index ;
					  int level = levelAndIndex.level ;
					  games[level]->players[index]->score = (cardVal%13)+1 ;
					  games[level]->players[index]->isActive = false ;
					  char setcard[3] ;
					  setcard[0] = (cardVal/10) + '0' ;
					  setcard[1] = (cardVal%10) + '0' ;
					  setcard[2] = '\0' ;
	               printf("%s got: %d of ", buffer, cardVal%13+1);
	               if(cardVal/13 == 0)
	                  printf("Spades\n");
	               else if(cardVal/13 == 1)
	                  printf("Clubs\n");
	               else if(cardVal/13 == 2)
	                  printf("Hearts\n");
	               else if(cardVal/13 == 3)
	                  printf("Diamonds\n");

					  if (send(clientfds[j], setcard, 3, 0)< 0)
						   printf("Error sending message");

				} // if buff
				else if(buffer[255] == '1')
				{
					isCardsGiven = true ;
					printf("%d ready\n", clientfds[j]);

					if (send(clientfds[j], "turn", 256, 0)< 0)
						error("Error sending turn");

					while(recv(clientfds[j], buffer,1024, 0) <= 0)
					  continue;


					Pair levelAndIndex = findPlayer(j) ;
					int index = levelAndIndex.index ;
					int level = levelAndIndex.level ;
					fflush(stdout) ;
					if(strcmp(buffer, "1") == 0)
					{
						printf("%s has bet %d coins.\n", games[level]->players[index]->name, (50)*(games[level]->players[index]->level + 1)) ;
						games[level]->players[index]->isActive = true ;
					}
					else if(strcmp(buffer, "0") == 0)
					{
						printf("%s didn't bet.\n", games[level]->players[index]->name) ;
					}
					else
					{
						printf("Wrong Option\n") ;
					}
					char* name = games[level]->players[index]->name ;
					int indexTemp ;
					buffer[1] = games[level]->players[index]->level + '0';
					for(indexTemp = 2; name[indexTemp-2] != '\0'; ++indexTemp)
					{
						buffer[indexTemp] = name[indexTemp-2] ;
					}
					int u =0;
					int coinsTemp = games[level]->players[index]->coins ;
					buffer[100] = coinsTemp/1000 + '0' ;
					coinsTemp %= 1000 ;
					buffer[101] = coinsTemp/100 + '0' ;
					coinsTemp %= 100 ;
					buffer[102] = coinsTemp/10 + '0' ;
					coinsTemp %= 10 ;
					buffer[103] = coinsTemp + '0' ;
					for(u =0; u < ncc; u++)
					{
					  	send(clientfds[u], buffer, 256, 0);
					}
					buffer[100] = buffer[101] = buffer[102] = buffer[103] = '\0' ;
					check[j] = 1;  
							 

					//break;
				}
				else
				{
				   
					printf("ELSE : hehehe%s\n", buffer);
				 
				}
			}
		} //if fd_isset
		FD_SET(clientfds[j+1], &readfds);
		j++;
	  } // for j=0

	} // else
  }
  return 0;
}
