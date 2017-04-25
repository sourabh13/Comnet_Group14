#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void sendWantGame(int fd, char *buffer) {
   // send the want game request
    int n;
    printf("before want game request\n");
    n = send(fd, buffer, 256, 0);

    if (n < 0)
         error("Error sending message");
    printf("sent a want game request\n");
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    if ( argc < 3) {
         error("hostname and port required");
    }
    char buffer[256];
    portno = atoi(argv[2]);

    // create socket and validate
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    printf("opened socket\n");

    // get server addr
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
   (char *)&serv_addr.sin_addr.s_addr,
   server->h_length);
    serv_addr.sin_port = htons(portno);
    printf("before connect\n");
    // connect to server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    printf("connected to server\n");
    bzero(buffer, 255);

 
    printf("Enter your name\n");
    char command[256];
    scanf("%s", buffer);
    buffer[255] = '0';
    //if (strcmp(command, "start") == 0) 
        sendWantGame(sockfd, buffer);
    // wait for a start game packet

    int card ;

    char getCard[52];

    if(recv(sockfd, getCard,3, 0) < 0)
   {
      printf("Error\n");
   } 
   else
   {
      // int i=0;
      // for(i=0; i<52; i++)
      // {
      //   if(getCard[i] != 0)
      //     printf("%d\n", i);
      // }
      int cardVal = (getCard[0]-'0')*10 + getCard[1]-'0' ;
      printf("The card is : %d of ", cardVal%13+1);
      if(cardVal/13 == 0)
         printf("Spades\n");
      else if(cardVal/13 == 1)
         printf("Clubs\n");
      else if(cardVal/13 == 2)
         printf("Hearts\n");
      else if(cardVal/13 == 3)
         printf("Diamonds\n");
   }
   int roundCount = 0 ;

   buffer[255] = '1';
   if(send(sockfd, buffer, 256, 0)< 0)
      printf("Ready error\n");
  
   while(1)
   { 
       n = recv(sockfd, buffer, 256, 0);
       
       if(n<0 || buffer[0] == '\0')
           continue;
       else
       {
         // send(sockfd, buffer, 256, 0) ;
         if (strcmp(buffer, "turn") == 0) 
         {
            
            if(roundCount%2 == 0)
            {
               printf("Do you want to bet(Yes = 1 & No = 0) :  ");
              char bet[20];
              memset(bet, '\0', sizeof(bet)) ;
              scanf("%s",bet);
              ++roundCount ;
              n = send(sockfd, bet, 20, 0);

              if(n < 0)
              {
                printf("client error\n");
                break;
              }    
            }
            else
            {
               printf("Next round.\n") ;
               char bet[20];
              memset(bet, '\0', sizeof(bet)) ;
              bet[0] = 'N' ;
              ++roundCount ;
              n = send(sockfd, bet, 20, 0);
              if(n < 0)
              {
                printf("client error\n");
                break;
              }   
              char getCard[52];

               while(recv(sockfd, getCard,7, 0) <= 0)
               {
                  // printf("Error\n");
                  continue ;
               }  
               int cardVal = (getCard[0]-'0')*10 + getCard[1]-'0' ;
               if(getCard[2] == '1')
               {
                  printf("You won the previous round.\n");
               }
               else if(getCard[2] == '0')
               {
                  printf("You lost the previous round.\n");
               }

               int coinsTemp = 0 ;
               coinsTemp += (getCard[3]-'0')*1000 ;
               coinsTemp += (getCard[4]-'0')*100 ;
               coinsTemp += (getCard[5]-'0')*10 ;
               coinsTemp += (getCard[6]-'0')*1 ;
               printf("You have %d coins left with you\n", coinsTemp);
               printf("The card you got is : %d of ", cardVal%13+1);
               if(cardVal/13 == 0)
                  printf("Spades\n");
               else if(cardVal/13 == 1)
                  printf("Clubs\n");
               else if(cardVal/13 == 2)
                  printf("Hearts\n");
               else if(cardVal/13 == 3)
                  printf("Diamonds\n");
            }
            
         }
         else
         {
            if(buffer[123] != '0')
            {
               char* name = (char*)calloc(20, sizeof(char)) ;
               int index ;
               for(index = 2; buffer[index] != '\0'; ++index)
                  name[index-2] = buffer[index] ;
               int coinsTemp = 0 ;
               coinsTemp += (buffer[100]-'0')*1000 ;
               coinsTemp += (buffer[101]-'0')*100 ;
               coinsTemp += (buffer[102]-'0')*10 ;
               coinsTemp += (buffer[103]-'0')*1 ;
               if(buffer[0] == '0')
               {
                  printf("%s with %d coins is not betting.\n", name, coinsTemp) ;
               }
               else
               {
                  printf("%s with %d coins is betting : %d\n", name, coinsTemp, (buffer[1] - '0' + 1)*50 ) ;
               }
            }
            // else
            // {  char* name = (char*)calloc(20, sizeof(char)) ;
            //    int index ;
            //    for(index = 2; buffer[index] != '\0'; ++index)
            //       name[index-2] = buffer[index] ;
            //    printf("%s is on different level betting.\n", name) ;
            // }
         }

   } 
}       





    
    close(sockfd);
    return 0;
}
