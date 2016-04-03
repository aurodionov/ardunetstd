#include <string.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <termios.h>  
#include <stdio.h>  
#include <signal.h>  
#include <unistd.h>  
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/wait.h> 

#define BUFSIZE 2048

char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

char index_file[BUFSIZE]={0,};
char index_filepatch[128]={0,};

char bRead[BUFSIZE] = {0,}; 
char er_log_str[32]={0,};
char device[32]={0,};
char file_ardu_patch[128]={0,};
char str_iz_file[BUFSIZE] = {0,};
char adr_udp[24] = {0,};

unsigned long int speedport = 0;
unsigned int PORTR = 0; 
unsigned int PORTS = 0; 

int udp_s = 0; 
int udp_c = 0; 

unsigned int PORTW = 0;  
int web = 0; 
unsigned long int PAUSARD = 0;


struct sockaddr_in server;
struct hostent *host; 

int sockfd;
pid_t udp_server;
pid_t web_server;
pid_t web_server2;


void error_log() 
 {  
   time_t t;
   time(&t);
   FILE *f;
   f = fopen("Error.log", "a"); 
   fprintf(f, "%s. ", er_log_str);
   fprintf(f, "%s", ctime( &t));  
   printf("Write to Error.log\n");
   fclose(f);
   kill(udp_server, SIGKILL);
   kill(web_server, SIGKILL);
   exit(0);
 }


void send_by_udp() 
 {
   if(sendto(sockfd, str_iz_file, strlen(str_iz_file), 0, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0)
     {
       strncpy(er_log_str, "ERROR_udp_send", 31);
       error_log();
     }

   printf("UDP_send: %s\n\n", str_iz_file);
 }


void read_index_file() 
 { 
   char result_sting[BUFSIZE]; 
   FILE *file; 
   file = fopen(index_filepatch,"r");

   if(file == NULL)
    {
      strncpy(er_log_str, "Error-Open index_file", 31);
      error_log();
    }

   while(fgets(result_sting,sizeof(result_sting),file))
    {
      strcat(index_file, result_sting);
    }

   fclose(file);
 }


int open_port()  
 {  
   int fd;  
   fd = open(device, O_RDWR | O_NOCTTY); 
 
   if(fd == -1) 
     {
       strncpy(er_log_str, "Error - NOT open port", 31);
       error_log();
     }

   else  
     {  
       struct termios options;  
       tcgetattr(fd, &options);   

       switch(speedport)
       {
	case 4800:       
          cfsetispeed(&options, B4800); 
          cfsetospeed(&options, B4800); 
        break;

	case 9600:       
          cfsetispeed(&options, B9600); 
          cfsetospeed(&options, B9600); 
        break;

	case 19200:       
          cfsetispeed(&options, B19200); 
          cfsetospeed(&options, B19200); 
        break;

	case 38400:       
          cfsetispeed(&options, B38400); 
          cfsetospeed(&options, B38400); 
        break;

	case 57600:       
          cfsetispeed(&options, B57600); 
          cfsetospeed(&options, B57600); 
        break;

	case 115200:       
          cfsetispeed(&options, B115200); 
          cfsetospeed(&options, B115200); 
        break;

	default: 
        printf("Error - Speed_port\n");
        strncpy(er_log_str, "Error - Speed_port\n", 31);
        error_log();
        break;
       }

       options.c_cflag |= (CLOCAL | CREAD); 
       options.c_cflag &= ~PARENB;  
       options.c_cflag &= ~CSTOPB;  
       options.c_cflag &= ~CSIZE;  
       options.c_cflag |= CS8;  
       options.c_cc[VMIN] = 1;  
       options.c_cc[VTIME] = 1;  
       options.c_lflag = ICANON;  
       options.c_oflag = 0;  
       options.c_oflag &= ~OPOST; 
       tcflush(fd, TCIFLUSH);
       tcsetattr(fd, TCSANOW, &options);  
     }  

   return fd;  
 }


int main(int argc, char *argv[])  
 {  
   if(argc != 13) 
    {
      printf("Not argumets!\n");
      strncpy(er_log_str, "Not argumets", 31);
      error_log();
    }
  
   strncpy(device, argv[1], 31); // arduina
   speedport = strtoul(argv[2], NULL, 0); // baud rate
   strncpy(file_ardu_patch, argv[3], 127); // путь к файлу ardu
   PORTR = strtoul(argv[4], NULL, 0); // порт для upd-сервера 3495 (приём)
   PORTS = strtoul(argv[5], NULL, 0); // порт для upd-клиента 3496 (отправка)
   strncpy(adr_udp, argv[6], 23); // адрес udp-сервера для отправки
   udp_s = atoi(argv[7]); // запуск udp-сервера ( 0 - не запускать, 1 - запускать)
   udp_c = atoi(argv[8]); // запуск udp-клиента ( 0 - не запускать, 1 - запускать)
   PORTW = strtoul(argv[9], NULL, 0); // порт для web-сервера 8080
   strncpy(index_filepatch, argv[10], 127); // путь к файлу index.html
   web = atoi(argv[11]); // запуск web-сервера ( 0 - не запускать, 1 - запускать)
   PAUSARD = strtoul(argv[12], NULL, 0); // пауза перед приёмом


   FILE *f;
   f = fopen(file_ardu_patch, "w"); 
   if(f == NULL) 
    {
      strncpy(er_log_str, "Error-Create file", 31);
      error_log();
    }

   fclose(f);


   int fd = open_port(); 
   sleep(2);
   tcflush(fd, TCIFLUSH);


////////////////////////////////// udp_server //////////////////////////////////////////
   if(udp_s) 
    {
      udp_server = fork();

      if(-1 == udp_server) 
       {
        strncpy(er_log_str, "Not udp_PID", 31);
        error_log();
       }

     if(udp_server == 0) 
       {
        char msg[BUFSIZE] = {0,};
        char to_Ardu[BUFSIZE] = {0,};
        
	memset(&server, 0x00, sizeof(server));  
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(PORTR);

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
         {
           strncpy(er_log_str, "Not sockfd udp_server", 31);
           error_log();
         }
 
        	
        if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)))
         {
           strncpy(er_log_str, "Not bind udp_server", 31);
           error_log();
         }
 
        while(1)
         {
           memset(msg, 0, sizeof(msg));
           memset(to_Ardu, 0, sizeof(to_Ardu));
           int n = recvfrom(sockfd, msg, BUFSIZE - 1, 0, NULL, NULL); // udp reciv and send to ardu
           msg[n] = 0;
           printf("UDP_reciv\n");

           snprintf(to_Ardu, BUFSIZE - 1, "echo '%s' > %s", msg, device);
           system(to_Ardu);

           printf("UDP_send_to_ardu: %s\n", msg);
         }
 
        close(sockfd);
        return EXIT_SUCCESS;

      } // END FORK udp
    }


////////////////////////////////// web_server //////////////////////////////////////////
if(web) 
 {
  web_server = fork();

  if(-1 == web_server) 
    {
      strncpy(er_log_str, "Not web_PID", 31);
      error_log();
    }  


  if(web_server == 0) 
    {
      read_index_file();

      int n2 = 1, client_fd;
      struct sockaddr_in svr_addr, cli_addr;
      socklen_t sin_len = sizeof(cli_addr);
 
      int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if(sock < 0) 
       {
         close(sock);
         strncpy(er_log_str, "Error - socket web", 31);
         error_log();
       }
  
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &n2, sizeof(n2));

      svr_addr.sin_family = AF_INET;
      svr_addr.sin_addr.s_addr = INADDR_ANY;
      svr_addr.sin_port = htons(PORTW); 
 
      if(bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) 
       {
         close(sock);
         strncpy(er_log_str, "Error - web bind", 31);
         error_log();
       }
 
      if(listen(sock, 5) == -1) 
       {
         close(sock);
         strncpy(er_log_str, "Error - listen", 31);
         error_log();
	}


      char buffer[BUFSIZE] = {0,};
      char bufRec[BUFSIZE] = {0,};

     while(1) 
     {
      client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);

      if(client_fd == -1) 
       {
         continue;
       }
 

      web_server2 = fork();

      if(-1 == web_server2) 
       {
         strncpy(er_log_str, "Not web_PID2", 31);
         error_log();
       }  

      if(web_server2 == 0) 
       {
         unsigned int i = 0;
         unsigned int ir = 0;

         memset(bufRec, 0, sizeof(bufRec));
         memset(buffer, 0, sizeof(buffer));

         read(client_fd, buffer, BUFSIZE - 1);
         //printf("buffer: %s\n", buffer);

         if((strstr(buffer, "GET")) != NULL)
         {
          if((strstr(buffer, "curl")) != NULL) //////////////////////////// Curl
            {
              printf("OK_Curl\n");

              for(; i <= strlen(buffer); i++)
               {
                 if(buffer[i] == '/')
                  {
                    i++;
                    for(; i <= strlen(buffer); i++)
                      {
                        bufRec[ir] = buffer[i];
                        ir++;

                        if(buffer[i] == '\r' || buffer[i] == '\n')
                          {
                            bufRec[ir] = 0; 
                            break;
                          }
                      }

                    break; 
                  }
               }
            

              memset(buffer, 0, sizeof(buffer));
              char *p;

              if((p = strstr(bufRec, "HTTP")) != NULL)
               {   
                 int index = p - bufRec;
                 int i = 0;
                 for(; i <= index - 2; i++)
                  {
                    buffer[i] = bufRec[i];
                  }

                 buffer[i] = 0;
                 memset(bufRec, 0, sizeof(bufRec));
                 snprintf(bufRec, BUFSIZE - 1, "echo '%s' > %s", buffer, device);
                 system(bufRec);
                 printf("Curl_send_toArdu: %s\n", bufRec);
               }

              memset(bufRec, 0, sizeof(bufRec));
              memset(buffer, 0, sizeof(buffer));

              usleep(PAUSARD);

              FILE *f; 
              f = fopen(file_ardu_patch, "r"); // файл ардуины
              if(f == NULL) 
               {
                 close(client_fd);
	         strncpy(er_log_str, "Error-NOT open file", 31);
                 error_log();
               } 

              fgets(buffer, BUFSIZE - 1, f);
              fclose(f); 

              snprintf(bufRec, BUFSIZE - 1, "%s\r\n", buffer);
              write(client_fd, bufRec, sizeof(bufRec));
              close(client_fd); 
              printf("Send to curl_client: %s\n", bufRec); 
              exit(0);
            }

          else //////////////////////////////////////////////////////////////// GET
            {
              printf("OK_GET\n");

              for(; i <= strlen(buffer); i++)
               {
                 if(buffer[i] == '=')
                  {
                    i++;
                    for(; i <= strlen(buffer); i++)
                      {
                        bufRec[ir] = buffer[i];
                        ir++;

                        if(buffer[i] == '\r' || buffer[i] == '\n')
                          {
                            bufRec[ir] = 0; 
                            break;
                          }
                      }

                    break; 
                  }
               }
            

              memset(buffer, 0, sizeof(buffer));
              char *p;

              if((p = strstr(bufRec, "HTTP")) != NULL)
               {   
                 int index = p - bufRec;
                 int i = 0;
                 for(; i <= index - 2; i++)
                  {
                    buffer[i] = bufRec[i];
                  }

                 buffer[i] = 0;
                 memset(bufRec, 0, sizeof(bufRec));
                 snprintf(bufRec, BUFSIZE - 1, "echo '%s' > %s", buffer, device);
                 system(bufRec);
                 printf("GET_send_toArdu: %s\n", bufRec);
               }
           }
         }
  
       else 
        {
          printf("NOT GET or Curl\n");
          close(client_fd);
          exit(0);
        }


       memset(bufRec, 0, sizeof(bufRec));
       memset(buffer, 0, sizeof(buffer));

       usleep(PAUSARD);

       FILE *f; 
       f = fopen(file_ardu_patch, "r"); // файл ардуины

       if(f == NULL) 
        {
          close(client_fd);
	  strncpy(er_log_str, "Error-NOT open file", 31);
          error_log();
        } 

       fgets(buffer, BUFSIZE - 1, f);
       fclose(f); 

       snprintf(bufRec, BUFSIZE - 1, "%s", buffer);
       write(client_fd, response, sizeof(response) - 1);
       write(client_fd, index_file, BUFSIZE); 
       write(client_fd, bufRec, BUFSIZE);
       close(client_fd);
       printf("Send to web_client: %s\n", bufRec);
       exit(0);

     } // END FORK web2

     close(client_fd);
     wait(NULL);

   } // END while

 } // END FORK web

} // END web
    

/////////////////////////////////////////// SEND UDP //////////////////////////////////////////////

   if((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) 
     {
       close(sockfd);
       strncpy(er_log_str, "Not sockfd", 31);
       error_log();
     } 

   int n1 = 1;
   setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&n1,sizeof(n1));

   host = gethostbyname(adr_udp);
   memset(&server, 0, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_port = htons(PORTS);
   server.sin_addr = *((struct in_addr*) host->h_addr);

/////////////////////////////////////////// READ ARDUINO //////////////////////////////////////////////

   unsigned int i;

   while(!VINTR) 
    {  
      int bytes = 0;
      memset(bRead, 0, sizeof(bRead));

      if((bytes = read(fd, bRead, BUFSIZE))==-1) // read()
        {
          printf("Error_Read_from_Arduino\n");
        }

      for(i = 0; i <= strlen(bRead); i++)
       {
         if(bRead[i] == '\n')
          {
            bRead[i] = 0; 
            break;
          }
       } 

     //////////////////// Сравнение строк //////////////////////
     if(strcmp(bRead, str_iz_file)==0)
       {
         printf("Str_equally\n"); 
       }

     else
       { 
         printf("Str_NOT_equally\n"); 
         FILE *f;
         f = fopen(file_ardu_patch, "w"); 
         if(f == NULL) 
          {
	    strncpy(er_log_str, "Error-Write to file", 31);
            error_log();
          }

         fprintf(f, "%s", bRead);
         fclose(f);
         memcpy(str_iz_file, bRead, sizeof(str_iz_file)); 
       }
    
     printf("Reciv_from_arduino: %s | Bytes - %d\n", bRead, bytes);

     if(udp_c)
      {
        send_by_udp();
      }
     
    } // END (while) 
 
  return 0;

 } // END main()


// gcc -Wall -Wextra ardunetstd.c -o ardunetstd

// ./ardunetstd /dev/ttyUSB0 57600 /tmp/tmp.txt 3495 3496 "192.168.5.224" 1 1 8080 /home/dima/index.web 1 300000

// 1 - путь к ардуине, 2 - baud rate, 3 - путь к файлу с данными, 4 - порт для upd-сервера 3495 (приём), 5 - порт для upd-клиента 3496 (отправка), 6 - адрес udp-сервера для отправки (писать в кавычках - "192.168.165.254"), 7 - запуск udp-сервера ( 0 - не запускать, 1 - запускать), 8 - запуск udp-клиента ( 0 - не запускать, 1 - запускать), 9 - порт для web-сервера 8080, 10 - путь к файлу index.html (/home/dima/index.html), 11 - запуск web-сервера ( 0 - не запускать, 1 - запускать), 12 - пауза перед отправкой ответа на web-страничку (200000)  

//  make package/ardunetstd/compile V=s

// /opt/ardunetstd /dev/ttyUSB1 57600 /tmp/tmp.txt 3495 3496 "192.168.5.224" 1 1 8080 /opt/index.web 1 300000






