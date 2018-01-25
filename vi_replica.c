/************************************************     PARUL JAIN   20172107      *****************************************************************/

#include<termios.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<string.h>
#include<sys/wait.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/unistd.h>

/***************************************************************linklist of character array*******************************************************/

int iflag=0; int dflag=0; int bflag=0; int rflag=0;
int switch1=0;
int j=0,rw=1;
typedef struct node 
{
   int i;
   char data[2000];
   struct node *next;
}nd; 
nd *head=NULL;

void insert(char a[])
{
  nd* new=(nd*)malloc(sizeof(nd));
  strcpy(new->data,a);
  new->i=rw;
  rw++;
  new->next=NULL; 
  if(head==NULL)
     head=new;
  else
  {
     nd *temp = head;
     while(temp->next!=NULL)
     {
          temp=temp->next;
     }
     temp->next=new;
	
  }
}

void print()    
{
   nd *temp=head;
   while(temp!=NULL)
   { 
     fprintf(stderr,"%s\n",temp->data);
     temp=temp->next;  
   }
}


/*********************************************writefile****************************************************************************************/

void writefile(char *f)
{
    FILE *file=fopen(f,"w");
    nd *temp=head;
    while(temp!=NULL)
   { 
	fprintf(file,"%s\n",temp->data);
     	temp=temp->next;  
   }
      	//fprintf(stderr,"\n");
    fclose(file);
}

/********************************************file input*****************************************************************************************/

int isfile(char *fname)
{
   FILE *f = fopen(fname,"r");
   if(f){
	fclose(f);
     return 1;
	}
   else
     return 0;
}

void chararr(char *a,int n)
{
	char *b=(char*)malloc(sizeof(char)*n);
	strcpy(b,a);
	insert(b);
}

void fileReader(char *file){
	FILE *f = fopen(file,"r");
	char *line=NULL;
	size_t read,len=0;
	while((read = getline(&line,&len,f))!=-1)
	{
		line[read-1]='\0';
		chararr(line,read-1);
	}
	fclose(f);
	print();
	
}
    
/*****************************************************************terminal************************************************************************/

struct config
{
 int  x,y;
 int rows,cols;
 struct termios orig;
};

struct config g;

void originalread()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &g.orig);
}

void readbyte()
{
  tcgetattr(STDIN_FILENO, &g.orig);
  atexit(originalread);
  struct termios raw=g.orig;
  raw.c_iflag &= ~(IXON  | BRKINT | INPCK | ISTRIP);
  raw.c_lflag &= ~( ICANON | ISIG | IEXTEN | ECHO);
  raw.c_cflag |= (CS8);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/******************************************************cursor movement**************************************************************************/

void mvcurse(int key) {
    switch (key) {
    case 'h':
      if (g.x != 0) 
      {
        g.x--;
      }
      break;
    case 'l':
      if (g.x != g.cols)
      {
        g.x++;
      }
      break;
    case 'k':
      if (g.y != 0)
      {
        g.y--;
      }
      break;
    case 'j':
      if (g.y != g.rows) 
      {
        g.y++;
      }
      break;
  }
  fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
}

void cursepos(int *rows, int *cols)
{
  char buf[32];
  unsigned int i = 0;
  write(STDOUT_FILENO, "\x1b[6n", 4);
  while (i < sizeof(buf) - 1) 
  {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  sscanf(&buf[2], "%d;%d", rows, cols) ;
}

int wsize(int *rows, int *cols) 
{
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) 
  {
     return -1;
  } 
  else
  {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

void tildes()
{
  int y;
  for (y = 0; y < g.rows-2; y++) 
    {
      write(STDOUT_FILENO, "\n", 3);
    }
    if (y < g.rows-2)
    {
      write(STDOUT_FILENO, "~", 1);
    }
  
}

/****************************************************************char by char reading*************************************************************/

char readkey() 
{
  char c;
  read(STDIN_FILENO, &c, 1);
  if(c=='\x1b')
  {
	char seq[3];
	if(read(STDIN_FILENO,&seq[0],1)!=1)
		return '\x1b';
	if(read(STDIN_FILENO,&seq[1],1)!=1)
		return '\x1b';
	if(seq[0]=='[')
	{
		if(seq[1]>='0' && seq[1]<='9')
		{
		  if(read(STDIN_FILENO,&seq[2],1)!=1)
		     return '\x1b';
		  if(seq[2]=='~')
		  {
			switch(seq[1])
			{
			case '3': return 1;                    
			}
		  }
                }
		else
		{
			switch(seq[1])
			{
			case 'A': return 'k';
			case 'B': return 'j';
			case 'C': return 'l';
			case 'D': return 'h';
			}
		}
	}
	return '\x1b';
  }
  else
  	return c;
}

/*********************************************************clear screen***************************************************************************/

void clearscreen()
{
	write(STDOUT_FILENO, "\x1b[2J",4);
	write(STDOUT_FILENO, "\x1b[H", 3);
 	tildes();
 	char buff[32];
 	snprintf(buff, sizeof(buff), "\x1b[%d;%dH", g.x+1, g.y+1);
 	write(STDOUT_FILENO,buff,strlen(buff));
 	write(STDOUT_FILENO, "\x1b[H", 3);
} 

/***********************************************************************INPUT MODE****************************************************************/

void forward_del()
{	
	dflag=1;
	cursepos(&g.y,&g.x);
	nd *temp=head;
	while(temp->i!=g.y)
	temp=temp->next;
	int len=strlen(temp->data);
	memmove(&(temp->data[g.x-1]), &(temp->data[g.x]),len-(g.x-1));
	fprintf(stderr,"\033[2K\r");
	fprintf(stderr,"%s",temp->data);
	fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
}

void backward_del()
{	
	bflag=1;
	cursepos(&g.y,&g.x);
	nd *temp=head;
	while(temp->i!=g.y)
	temp=temp->next;
	int len=strlen(temp->data);
	memmove(&(temp->data[g.x-2]), &(temp->data[g.x-1]),len-(g.x-1));
	fprintf(stderr,"\033[2K\r");
	fprintf(stderr,"%s",temp->data);
	fprintf(stderr,"\x1b[%d;%dH",g.y,g.x-1);
}

nd* makechange(nd *temp1,int x,int y,char c)
{
	int j=0;
	char s[50];
	x=y-1;
	while(temp1->data[x]!='\0')
	{
		s[j]=temp1->data[x];
		j++;
		s[j]='\0';
		x++;
	}
	x=y-1;
	if(c!='\n')
		temp1->data[x++]=c;
	j=0;
	while(j<strlen(s))
	{
		temp1->data[x++]=s[j];
		j++;
		temp1->data[x+j]='\0';
	}																		return temp1;	
}

int input(){

    cursepos(&g.y,&g.x);
    fprintf(stderr,"\x1b[%d;%dH",g.rows,0);
    fprintf(stderr,"INPUT MODE");
    fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
    char c=readkey();
    int i=0;
	if(c=='\x1b')                                                        //go to normal mode
	{
	 switch1=0;
        }
  	else if(c==1)
	{
		forward_del();
	}
	else if(c==127)
	{
		backward_del();
	}
        else 
	{     
		iflag=1;
		cursepos(&g.y,&g.x);     
		nd *temp=head,*pre;
		int count=0;
		nd *new;
		int cx=0;
	   if(c!='\n')
	   {
		if(temp==NULL && g.y==1)
		{
			new=(nd*)malloc(sizeof(nd));
			new->next=NULL;
			temp=head=new;
			temp->data[cx]=c;
			temp->data[cx+1]='\0';
			temp->i=rw;
		}
		else
		{
		  while(count!=g.y-1 && temp!=NULL )
		  {
			pre=temp;
			temp=temp->next;
			count++;
		  }
			if(temp!=NULL)
			{
				cx=g.x-1;
				if(temp->data[cx]=='\0')
				{
					temp->data[cx++]=c;
					temp->data[cx]='\0';
				
				}
				else
				{        
					makechange(temp,cx,g.x,c);            
				}	
		        }
			else
			{
				new=(nd*)malloc(sizeof(nd));
				new->next=NULL;
				temp=pre->next=new;
				new->data[cx++]=c;
				new->data[cx]='\0';
				new->i=rw;
			}
		}
		fprintf(stderr,"\033[2K\r");
		fprintf(stderr,"%s",temp->data);
		fprintf(stderr,"\x1b[%d;%dH",g.y,g.x+1);
     	   }	
     	   else
     	   {
		rw++;
		fprintf(stderr,"%c",c);
     	   }
	}
	return 0;
}

/******************************************************************COMMAND MODE*****************************************************************/

int command(char *f)
{

	cursepos(&g.y,&g.x);
  	fprintf(stderr,"\x1b[%d;%dH",g.rows,0);
  	fprintf(stderr,"COMMAND MODE");
  	fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
	char d, c=readkey();
	fprintf(stderr,"\x1b[%d;%dH%c",g.rows-2,2,c);
	char ch[20],e;
	char *args[3];
	int k=0;
     if(c=='!')
     {
	while(1)
 	{	
		read(STDIN_FILENO,&e,1);
		if(e!='\n')
		{
			fprintf(stderr,"\x1b[%d;%dH%c",g.rows-2,2,e);
			ch[k++]=e;
		}
		else
		break;
	}
	ch[k]='\0';
	args[1]=NULL;
	args[0]=ch;
	if(fork())
	{
		wait(0);
		read(STDIN_FILENO,&e,1);
		write(STDOUT_FILENO,"x1b2J",4);
		write(STDOUT_FILENO,"x1b[H",3);
		clearscreen();

	}
	else
	{	
		write(STDOUT_FILENO,"x1b2J",4);
		write(STDOUT_FILENO,"x1b[H",3);
		if(execvp(args[0],args)==-1)
			perror("exec");
	}
     }
     else
     {
	int i=0;
	  switch(c)
	  {
		case 'w':
			d=readkey();
			if(d=='\n')
			{	
				writefile(f);
				iflag==0 ;
				dflag==0 ;
				bflag==0;
				rflag==0;
			}
			break;
		case 'q':
			d=readkey();
			if(d=='\n')
			{
				if(iflag==1 || dflag==1 || bflag==1 || rflag==1)
				{
					fprintf(stderr,"\x1b[%d;%dH %s ",g.rows-1,1," No changes will be saved");
					return 0;
				}
				else
				{
					fprintf(stderr,"\x1b[%d;%dH",g.rows,1);
					exit(0);
				}
			}
			else if(d=='!')
			{
			  fprintf(stderr,"\x1b[%d;%dH%c",g.rows-2,3,d);
			  char e=readkey();
			  if(e=='\n')
		          {
				fprintf(stderr,"\x1b[%d;%dH",g.rows,1);
				exit(0);
			  }
			}
			
			break;
	  }
     }
}

/***************************************************************NORMAL MODE**********************************************************************/

void replace()
{
	rflag=1;
	cursepos(&g.y,&g.x);
	nd *temp=head;
	while(temp->i!=g.y)
	temp=temp->next;
	char c;
	read(STDIN_FILENO,&c,1);
	fprintf(stderr,"%c",c);
	temp->data[g.x]=c;
}

int normal(char *f)
{
  cursepos(&g.y,&g.x);
  fprintf(stderr,"\x1b[%d;%dH",g.rows,0);
  fprintf(stderr,"NORMAL MODE");
   fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
  char c = readkey();
  int i=0,z=0;
  switch (c)
  {
      case (':'):
      	i=1;
      	break;
      case 'k':
      case 'j':
      case 'h':
      case 'l':
	cursepos(&g.y,&g.x);
      	mvcurse(c);
      	break;
      case 'g':
      	z=1;
      	break;
      case 'G' :
	g.y=rw-1;
	g.x=0;
	fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
        break;
      case 'r':
	replace();
	break;
      default:
        if(c=='i')                                                         //go to input mode
	   switch1=2;
        break;
  }
  if(i==1)                                                                //go to command mode
  {
	fprintf(stderr,"\x1b[%d;%dH%c",g.rows-2,1,c);
	command(f);
  }
  if(z==1)
  {
  	char p=readkey();
	if(p=='g')
	{
		g.x=0;
		g.y=0;
		fprintf(stderr,"\x1b[%d;%dH",g.y,g.x);
	}
  }
  return 0;
}

/**************************************************************init******************************************************************************/

void init()
{
   g.x=0;
   g.y=0;
   if (wsize(&g.rows, &g.cols) == -1) 		
	fprintf(stderr,"wsize");
}

int main(int argv, char *argc[])
{
	readbyte();
	init();
        clearscreen();
        if(isfile(argc[1])) 					             
            fileReader(argc[1]);
	  while(1)
	  {
		if(switch1==0)
		{
		   int p=normal(argc[1]);
		}
		else if(switch1==2)
		{
		   int p=input();
		}
	 }
	return 0;
}
