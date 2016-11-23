#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>
#include "LineParser.h"
#include <fcntl.h>
#define size 10
#define STDOUT 1
#define STDIN 0

void redirection(cmdLine* CmdLine, int debug)
{
	int fin;
	int fout;
	if (CmdLine->inputRedirect != NULL)
	{
		fin = open(CmdLine->inputRedirect, O_RDWR, 0777);
		if (fin < 0)
		{
			if(debug)fprintf(stderr, "file open error\n");
			_exit(EXIT_FAILURE);
		}
		if (dup2(fin, 0) < 0)
		{
			if(debug)fprintf(stderr, "dup error\n");
			_exit(EXIT_FAILURE);
		}
	}
	if (CmdLine->outputRedirect != NULL)
	{
		fout = open(CmdLine->outputRedirect, O_RDWR | O_APPEND | O_CREAT, 0777);
		if (fout < 0)
		{
			if(debug)fprintf(stderr, "file open error\n");
			_exit(EXIT_FAILURE);
		}
		if (dup2(fout, 1) < 0)
		{
			if(debug)fprintf(stderr, "dup error\n");
			_exit(EXIT_FAILURE);
		}
	}
}

int *leftPipe(int **pipes, cmdLine *pCmdLine) 
{
	int index = pCmdLine->idx;
	if (0 < index)
	{
		return pipes[index-1];
	}
	return NULL;
}

int *rightPipe(int **pipes, cmdLine *pCmdLine) 
{
	int index = pCmdLine->idx;
	if (pCmdLine->next == NULL) return NULL;
	return pipes[index];
}

void first_write(int **pipes, cmdLine *pCmdLine ,int debug)
{
	int status1;
	pid_t childpid1;
	int* fd = rightPipe(pipes, pCmdLine);

	if(debug) fprintf(stderr, "parent_process>forking…\n");

	childpid1 = fork();

	if(childpid1 == -1)
	{
		perror("fork error");
		_exit(EXIT_FAILURE);
	}

	if(childpid1 == 0)
	{
		if(debug) fprintf(stderr, "child1>redirecting stdout to the write end of the pipe…\n");
		close(STDOUT);
		if (dup(fd[1]) < 0)
		{
			if(debug)fprintf(stderr, "dup error");
			_exit(EXIT_FAILURE);
		}

		close(fd[1]);

		redirection(pCmdLine, debug);
		if(debug) fprintf(stderr, "child1>going to execute cmd: %s\n", *(pCmdLine->arguments));
		if (execvp(pCmdLine->arguments[0], pCmdLine->arguments)==-1)
		{
			perror("execv() error");
			exit(-1);
		}
	}

	else
	{
		waitpid(childpid1, &status1, 0);
		if(debug) fprintf(stderr, "parent_process>created process with id:%i\n", childpid1);
		if(debug) fprintf(stderr, "parent_process>closing the write end of the pipe…\n");
        /* Parent process closes up write side of pipe */
		close(fd[1]);
	}
}

void read_and_write(int **pipes, cmdLine *pCmdLine ,int debug)
{

	int status2;
	pid_t childpid2;
	int* fd = leftPipe(pipes, pCmdLine);
	int* fd2 = rightPipe(pipes, pCmdLine);

	if(debug) fprintf(stderr, "parent_process>forking…\n");
	childpid2 = fork();

	if(childpid2 == -1)
	{
		perror("fork error");
		_exit(EXIT_FAILURE);
	}

	if(childpid2 == 0)
	{
		if(debug) fprintf(stderr, "child2>redirecting stdin to the read end of the pipe…\n");
		close(STDIN);
		if (dup(fd[0]) < 0)
		{
			if(debug)fprintf(stderr, "dup error");
			_exit(EXIT_FAILURE);
		}

		close(fd[0]);

		if(debug) fprintf(stderr, "child1>redirecting stdout to the write end of the pipe2…\n");
		close(STDOUT);
		if (dup(fd2[1]) < 0)
		{
			if(debug)fprintf(stderr, "dup error");
			_exit(EXIT_FAILURE);
		}

		close(fd2[1]);

		redirection(pCmdLine, debug);
		if(debug) fprintf(stderr, "child2>going to execute cmd:%s\n", *(pCmdLine->arguments));
		if (execvp(pCmdLine->arguments[0], pCmdLine->arguments)==-1)
		{
			perror("execv() error");
			exit(-1);
		}
	}

	else
	{
		waitpid(childpid2, &status2, 0);
		if(debug) fprintf(stderr, "parent_process>created process with id:%i\n", childpid2);
		if(debug) fprintf(stderr, "parent_process>closing the read end of the pipe…\n");
		if(debug) fprintf(stderr, "parent_process>closing the write end of the pipe2…\n");
        /* Parent process closes up read side of pipe */
		close(fd[0]);
		close(fd2[1]);

	}
}

void last_read(int **pipes, cmdLine *pCmdLine ,int debug)
{
	int status2;
	pid_t childpid2;
	int* fd = leftPipe(pipes, pCmdLine);

	if(debug) fprintf(stderr, "parent_process>forking…\n");
	childpid2 = fork();

	if(childpid2 == -1)
	{
		perror("fork error");
		_exit(EXIT_FAILURE);
	}

	if(childpid2 == 0)
	{
		if(debug) fprintf(stderr, "child2>redirecting stdin to the read end of the pipe…\n");
		close(STDIN);
		if (dup(fd[0]) < 0)
		{
			if(debug)fprintf(stderr, "dup error");
			_exit(EXIT_FAILURE);
		}

		close(fd[0]);

		redirection(pCmdLine, debug);
		if(debug) fprintf(stderr, "child2>going to execute cmd:%s\n", *(pCmdLine->arguments));
		if (execvp(pCmdLine->arguments[0], pCmdLine->arguments)==-1)
		{
			perror("execv() error");
			exit(-1);
		}
	}

	else
	{
		waitpid(childpid2, &status2, 0);
		if(debug) fprintf(stderr, "parent_process>created process with id:%i\n", childpid2);
		if(debug) fprintf(stderr, "parent_process>closing the read end of the pipe…\n");
        /* Parent process closes up read side of pipe */
		close(fd[0]);
		
		/*
		waitpid(childpid1, &status11, WUNTRACED|WCONTINUED);
		waitpid(childpid2, &status21, WUNTRACED|WCONTINUED);
		*/

		if(debug) fprintf(stderr, "parent_process>exiting…\n");
	}
}

int countCommands(cmdLine* CmdLine)
{
	int i = 1;
	while(CmdLine->next)
	{
		++i;
		CmdLine = CmdLine->next;	
	}
	return i;
}

void free_pipes(int** pipes, int nPipes)
{
	int i;

	for (i = 0; i < nPipes; ++i)
	{		
		free(pipes[i]);
	}
	free(pipes);

}

int **createPipes(int nPipes) 
{
	if (nPipes <= 0)
	{
		return NULL;
	}

	int i;
	int **pipeArray;
	if ((pipeArray = (int**) calloc(nPipes, sizeof(int*))) == NULL)
	{
		fprintf(stderr, "malloc error");
		return NULL;
	}

	for (i = 0; i < nPipes; ++i)
	{		
		if ((pipeArray[i] = (int*) calloc(2, sizeof(int))) == NULL)
		{
			fprintf(stderr, "malloc error");
			return NULL;
		}
		else
		{
			if(pipe(pipeArray[i]) == -1)
			{
				perror("pipe error");
				_exit(EXIT_FAILURE);
			}
		}
	}

	return pipeArray;
}

void releasePipes(int **pipes, int nPipes)
{
	if (nPipes <= 0)
	{
		fprintf(stderr, "unvaild array size\n");
		return;
	}
	int i;
	for (i = 0; i < nPipes; ++i)
	{
		free(pipes[i]);
	}
	free(pipes);
}

typedef struct env_var env_var;

struct env_var {
	char* name;
	char* value;
};

typedef struct _link _link;

struct _link {
	env_var* env_var;
	_link* next;
	_link* prev;
};

void delete(_link* link)
{
	free(link->env_var->name);
	free(link->env_var->value);
	free(link->env_var);
	free(link);
	link = NULL;
}

/*append to the head of the list*/
_link* list_append(_link* head, _link* new_link)
{
	if (head != NULL)
	{
		new_link->next = head;
		head->prev = new_link;
	} 
	else 
		new_link->next = NULL;

	return new_link;
}

void print(_link *head)
{
	if (head == NULL)
		return;

	_link* curr_link = head;
	char* curr_name = NULL;
	env_var* curr_env_var;

	while(curr_link != NULL)
	{
		curr_env_var = curr_link->env_var;
		curr_name = curr_env_var->name;
		curr_link = curr_link->next;
	}
}

_link* search(_link* head, char* name)
{
	if (head == NULL)
		return NULL;

	_link* curr_link = head;
	char* curr_name;
	env_var* curr_env_var;

	while(curr_link != NULL)
	{
		curr_env_var = curr_link->env_var;
		curr_name = curr_env_var->name;
		
		if (strcmp(curr_name, name)==0)
		{
			return curr_link;
		}
		curr_link = curr_link->next;
	}
	return NULL;	
}

void list_free(_link* head)
{
	_link* curr = head;
	_link* temp = NULL;
	while(curr != NULL)
	{
		temp = curr->next;
		delete(curr);
		curr = temp;
	}
}


int min(int a, int b)
{
	if (a < b)
	{
		return a;
	}

	return b;
}

void execute(cmdLine *pCmdLine)
{ 
	if (execvp(pCmdLine->arguments[0], pCmdLine->arguments)==-1)
	{
		perror("execv() error");
		exit(-1);
	}
}

int main(int argc, char **argv)
{	
	int com_count;
	int pipes_num;
	int set_on = 0;
	char input[2048];
	_link* head = NULL;
	_link* ret;
	_link* ret2;
	char history[size][2048];
	int mini;
	int i;
	int com_num = 0;
	int pos = 0;
	pid_t pid;
	int debug = 0;
	int status;
	char cwd[PATH_MAX];
	cmdLine* input_cmdLine;
	char* quit = "quit\n";
	char* cd = "cd";
	int ret_v;
	char* hist = "history\n";
	char* set = "set";
	int num;
	char* name2 = (char*) malloc(100 * sizeof(char));
	char* env = "env";
	char* del = "delete";
	char* home = "~";

	if (argc > 1)
	{
		if(strcmp(argv[1], "-d") == 0){debug=1;}
		else return -1;
	}
	
	while(1)
	{	
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			fprintf(stdout, "Current working dir: %s\n", cwd);
		else
		{
			perror("getcwd() error");
			exit(-1);
		}

		fgets(input, sizeof(input), stdin);

		if (strcmp(input, quit)==0)
		{
			return 0;
		}

		if (input[0] != '!')
		{	
			if (com_num <= 9)
			{
				strcpy(history[pos],input);
				++pos;
				++com_num;
			}

			else
			{
				pos = 9;
				for (i = 0; i <= 8; ++i)
				{
					strcpy(history[i],history[i+1]);
				}
				strcpy(history[pos],input);
			}		

		}

		else if (input[0] == '!')
		{
			num = atoi(&input[1]);

			if (min(com_num,size) <= num)
			{
				if (debug){fprintf(stderr, "out of bounds number given\n");}
				continue;
			}

			else
			{
				strcpy(input,history[num]);

				if (com_num <= 9)
				{
					strcpy(history[pos],history[num]);
					++pos;
					++com_num;
				}

				else
				{
					for (i = 0; i <= 8; ++i)
					{
						strcpy(history[i],history[i+1]);
					}
					strcpy(history[pos],history[num-1]);
					
				}		
			}		
		}

		input_cmdLine = parseCmdLines(input);
		com_count = countCommands(input_cmdLine);

		if (com_count == 1)
		{
			for (i = 1; input_cmdLine->arguments[i] != NULL; ++i)
			{
				if (input_cmdLine->arguments[i][0] == '$')
				{
					strcpy(name2, &(input_cmdLine->arguments[i][1]));
					ret = search(head, name2);

					if (ret != NULL) 
					{
						replaceCmdArg(input_cmdLine, i, ret->env_var->value);
					}
					else {if (debug) fprintf(stderr, "Variable not found\n");}
				}	
			}

			if (strcmp(input, hist)==0)
			{
				mini = min(10, com_num);
				for (i = 0; i < mini; ++i)
				{
					printf("%d)%s",i, history[i]);
				}

				if (debug)
				{
					fprintf(stderr, "pid: %lld\n", (long long) getpid());
					fprintf(stderr, "executed command:%s", input);
				}
				freeCmdLines(input_cmdLine);
				continue;	
			}

			if (strcmp(input_cmdLine->arguments[0], del)==0 && (input_cmdLine->arguments[1] != NULL))
			{
				if (debug)
				{
					fprintf(stderr, "pid: %lld\n", (long long) getpid());
					fprintf(stderr, "executed command:%s", input);
				}

				ret2 = search(head, input_cmdLine->arguments[1]);

				if (ret2 == NULL)
				{
					if (debug) fprintf(stderr, "variable not found\n");
				}

				else
				{
					if (ret2->prev !=NULL){ret2->prev->next = ret2->next;}
					if (ret2->next !=NULL){ret2->next->prev = ret2->prev;}
					delete(ret2);
				}
				freeCmdLines(input_cmdLine);
				continue;
			}

			else if ((strcmp(input_cmdLine->arguments[0], cd)==0) && (input_cmdLine->arguments[1] != NULL))			
			{	
				if ((strcmp(input_cmdLine->arguments[1], home)==0))
				{
					ret_v = chdir(getenv("HOME"));
				}

				else
				{
					ret_v = chdir(input_cmdLine->arguments[1]);
				}

				if (ret_v == -1)
				{
					fprintf(stderr, "chdir command error\n");
				}

				if (debug && ret_v != -1)
				{
					fprintf(stderr, "pid: %lld\n", (long long) getpid());
					fprintf(stderr, "executed command:%s", input);
				}

				freeCmdLines(input_cmdLine);
				continue;
			}		

			else if ((strcmp(input_cmdLine->arguments[0], set)==0) && (input_cmdLine->arguments[1] != NULL) && (input_cmdLine->arguments[2] != NULL))
			{
				set_on = 1;
				if (debug)
				{
					fprintf(stderr, "pid: %lld\n", (long long) getpid());
					fprintf(stderr, "executed command:%s", input);
				}

				ret = search(head, input_cmdLine->arguments[1]);

				if (ret == NULL)
				{
					if(debug){fprintf(stderr, "Activating new environment variable\n");}
					char* name = (char*) malloc(100 * sizeof(char));
					char* value = (char*) malloc(100 * sizeof(char));
					strcpy(name, input_cmdLine->arguments[1]);
					strcpy(value, input_cmdLine->arguments[2]);
					env_var* ev = (env_var*) malloc (2 * sizeof(char*));
					ev->name = name;
					ev->value = value;
					_link* new_link = (_link*) malloc(sizeof(env_var*) + 2 * sizeof(_link*));
					new_link->env_var = ev;
					new_link->prev = NULL;
					head = list_append(head, new_link);

					if (head->next != NULL)
					{				
						printf("%s\n", head->next->env_var->name);
						printf("%s\n", head->next->env_var->value);
					}
					freeCmdLines(input_cmdLine);
					continue;
				}

				else
				{
					strcpy(ret->env_var->value, input_cmdLine->arguments[2]);
					freeCmdLines(input_cmdLine);
					continue;
				}

			}

			else if ((strcmp(input_cmdLine->arguments[0], env)==0) && (input_cmdLine->arguments[1] == NULL))
			{
				if (debug)
				{
					fprintf(stderr, "pid: %lld\n", (long long) getpid());
					fprintf(stderr, "executed command:%s", input);
				}
				if (head != NULL)
				{
					print(head);
				}
				else if (debug) fprintf(stderr, "The list is empty\n");

				freeCmdLines(input_cmdLine);
				continue;
			}

			pid = fork();

			if (pid == -1) {perror("fork() error"); _exit(EXIT_FAILURE);}

			if(pid==0)
			{		
				if (debug)
				{
					fprintf(stderr, "Child pid: %lld\n", (long long) getpid());
					fprintf(stderr, "executed command:%s", input);
				}
				redirection(input_cmdLine, debug);
				execute(input_cmdLine);
				_exit(EXIT_FAILURE);		
			}

			else
			{		
				waitpid(pid, &status, !input_cmdLine->blocking);
				if (debug)
				{
					fprintf(stderr, "Parent pid: %lld\n", (long long) getpid());
				}
				freeCmdLines(input_cmdLine);
			}
		}

		else
		{
			cmdLine* temp = input_cmdLine;
			pipes_num = com_count-1;
			int** pipes;
			pipes = createPipes(pipes_num);

			while(input_cmdLine->idx < com_count)
			{
				if (input_cmdLine->idx == 0)
				{
					first_write(pipes, input_cmdLine ,debug);
					input_cmdLine = input_cmdLine->next;
				}

				else if (0 < input_cmdLine->idx && input_cmdLine->idx < com_count-1)
				{
					read_and_write(pipes, input_cmdLine ,debug);
					input_cmdLine = input_cmdLine->next;
				}

				else if (input_cmdLine->idx == com_count-1)
				{
					last_read(pipes, input_cmdLine ,debug);
					break;
				}
			}
			free_pipes(pipes, pipes_num);
			freeCmdLines(temp);
		}
	}
	free(name2);
	if (set_on) list_free(head);
	return 0;
}
		/*
	cmdLine* input_cmdLine;
	char input[2048];
	fgets(input, sizeof(input), stdin);
	input_cmdLine = parseCmdLines(input);
	int* right;

	int** check = createPipes(3);
	int i;
	int j;

	if (check == NULL)
	{
		printf("NULL");
	}

	else
	{
		for (i = 0; i < 3; ++i)
		{
			printf("pipe number %d holds fd:\n", i+1);
			for (j = 0; j < 2; ++j)
			{
				printf("%d\n", check[i][j]);
			}
			printf("\n");
			
		}
	}

	left = leftPipe(check ,input_cmdLine->next);

	if (left == NULL)
	{
		printf("good\n");
	}
	

	right = rightPipe(check ,input_cmdLine->next);

	if (right == NULL)
	{
		printf("good\n");
	}
	
	else
	{
		printf("%i\n", right[0]);
		printf("%i\n", right[1]);
	}

	freeCmdLines(input_cmdLine);
	releasePipes(check, 3);
	*/

	/*
	int** check = createPipes(0);
	int i;
	int j;

	if (check == NULL)
	{
		printf("NULL");
	}

	else
	{
		for (i = 0; i < 3; ++i)
		{
			for (j = 0; j < 2; ++j)
			{
				printf("%d\n", check[i][j]);
			}
			
		}
	}
	*/