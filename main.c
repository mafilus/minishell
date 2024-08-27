#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <wordexp.h>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>



#define LOGIN_MAX_SIZE 256
#define CWD_MAX_SIZE 256
#define CMD_MAX_SIZE 256
#define PROMPTER_MAX_SIZE (CWD_MAX_SIZE + LOGIN_MAX_SIZE + 10)
#define SIZE_NUMBER 64

#define SET_STRING_UNKNOW(STR) \
  STR[0] = 'u'; \
  STR[1] = 'n'; \
  STR[2] = 'k'; \
  STR[3] = 'n'; \
  STR[4] = 'o'; \
  STR[5] = 'w'; \
  STR[6] = '\0';



static char prompter[PROMPTER_MAX_SIZE] = {0};
static char* commandline = NULL;
enum show_prompt {DEFAULT_PROMPT, NONE, CONFIG};

struct config_prompt {
  enum show_prompt cnf_out;
  char user_config[PROMPTER_MAX_SIZE];
};

char* prompt(struct config_prompt* conf)
{
  char login[LOGIN_MAX_SIZE];
  char cwd[CWD_MAX_SIZE];
  int errl;
  char* errc;

  // cleanup the prompter
  (void)memset(prompter,'\0', PROMPTER_MAX_SIZE);

  

  switch(conf->cnf_out){
  case DEFAULT_PROMPT:

    // get info
    errl = getlogin_r(login, LOGIN_MAX_SIZE);
    errc = getcwd(cwd, CWD_MAX_SIZE);
    
    // if login is null or cwd is null set unknow string
    if(errl)
      {
        SET_STRING_UNKNOW(login);
      }
    
    if(errc == NULL)
      {
        SET_STRING_UNKNOW(cwd);
      }
    (void)snprintf(prompter, PROMPTER_MAX_SIZE,"%s@%s>",login,cwd);
    break;
  case CONFIG:
    strncpy(prompter, conf->user_config, PROMPTER_MAX_SIZE);
    break;
  case NONE:
  default:
    prompter[0]='\0';
  }
  return prompter;
}

void _xfree(void **mem)
{
  if(mem != NULL && *mem != NULL)
    {
      free(*mem);
      *mem = NULL;
    }
}


struct config_gets {
  FILE* input;
  FILE* output;
  char* prompter;
};

char * commandline_gets (struct config_gets* conf)
{
  _xfree((void**)&commandline);

  if(conf->input != NULL)
    {
      rl_instream = conf->input;
      rl_outstream = conf->output;
    }

  commandline = readline (conf->prompter);

  if (commandline != NULL && commandline[0] != '\0')
    add_history (commandline);

  return (commandline);
}





void set_lastret(int ret)
{
  char lastret[SIZE_NUMBER];
  sprintf(lastret, "%d", ret);
  setenv("?",lastret,1);

}

int execute(char* path, char** argv)
{
  pid_t pid = fork();
  switch(pid)
    {
    case 0:
      (void)execvp(path, argv);
      (void)perror(path);
      (void)exit(EXIT_FAILURE);
      break;
    case -1:
      perror("fork");
      break;
    default:
      {
        int ret = 0;
        ret = waitpid(pid,&ret,0);
        set_lastret(ret);
      }        
      break;
    }
}

struct icmd{
  char name[CMD_MAX_SIZE];
  int (*main)(int argc, char** argv);
};

struct icmds{
  struct icmd** commands;
  size_t size;
};

static struct icmds* internalcmds = NULL;

int addinternalscmds(struct icmd* cmd){

  if(internalcmds == NULL){
    internalcmds = malloc(sizeof(struct icmds));
    if(internalcmds == NULL)
      return errno;
    internalcmds->size = 0;
    internalcmds->commands = malloc(sizeof(struct icmd*));
    if(internalcmds->commands == NULL)
      return errno;
  }


  {
    void* tmp = realloc(internalcmds->commands, (internalcmds->size + 1) * sizeof(struct icmd*));
    if(tmp == NULL)
      {
        internalcmds->size--;
        return errno;
      }

    internalcmds->commands = tmp;
  }

  internalcmds->commands[internalcmds->size] = cmd;
  internalcmds->size++;
  return 0;
}

int addcmd(char* cmd, int (*main)(int argc, char** argv))
{
  struct icmd* new_cmd = malloc(sizeof(struct icmd));
  memset(new_cmd->name,'\0',CMD_MAX_SIZE);
  strncpy(new_cmd->name, cmd, CMD_MAX_SIZE);
  new_cmd->main = main;
  addinternalscmds(new_cmd);
}

void freecmd()
{
  struct icmd* cur= NULL;
  cur = internalcmds->commands[0];
  for(size_t i = 0 ; i < internalcmds->size; ++i)
    {
      cur = internalcmds->commands[i];
      free(cur);
    }
  _xfree((void**)&internalcmds->commands);
  _xfree((void**)&internalcmds);
}

int run_internalcmd(struct icmds* internalcmds, char* cmd,int argc, char** argv)
{
  int ret = -1;
  int isrun = 0;

  for(size_t i = 0; i < internalcmds->size; i++)
    {
      struct icmd* cur = internalcmds->commands[i];
      if(!strcmp(cur->name,cmd)){
        isrun = 1;
        // run internal command
        ret = cur->main(argc,argv); 
        set_lastret(ret);
      }
    }
  return isrun;
}

int set_main(int argc, char** argv)
{
  if(argc != 3)
    {
      fprintf(stderr,"syntax: set name value\n");
      return EXIT_FAILURE;
    }

  if(setenv(argv[1],argv[2], 1))
    {
      perror("set");
    }
  return EXIT_SUCCESS;
}

int cd_main(int argc, char** argv)
{
  if(argc != 2)
    {
      fprintf(stderr,"syntax: cd path\n");
      return EXIT_FAILURE;
    }

  if(chdir(argv[1]))
    {
      perror(argv[1]);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}


void parse_print_err(int err)
{
  switch(err)
    {
    case WRDE_BADCHAR:
      fprintf(stderr,"Invalid character\n");
      break;
    case WRDE_BADVAL:
      fprintf(stderr,"Error variable\n");
      break;
    case WRDE_CMDSUB:
      fprintf(stderr,"Error sub command\n");
      break;
    case WRDE_NOSPACE:
      fprintf(stderr,"No free memory\n");
      break;
    case WRDE_SYNTAX:
      fprintf(stderr,"Error syntax\n");
      break;

    }
}


void parse(char* commandline)
{
  wordexp_t words;
  int err = 0;
  if((err = wordexp(commandline, &words, WRDE_SHOWERR)))
    {
      parse_print_err(err);
      return;
    }
  if((run_internalcmd(internalcmds,words.we_wordv[0], words.we_wordc ,words.we_wordv)) == 0)
    execute(words.we_wordv[0],words.we_wordv);
  
  wordfree(&words);
}




void interpreter(struct config_prompt* cnf_prompt, struct config_gets* cnf_gets)
{
  while(1)
    {
      char* cmdline = NULL;
      char* PS1 = prompt(cnf_prompt);
      cnf_gets->prompter = PS1;
      cmdline = commandline_gets(cnf_gets);
      if(feof(cnf_gets->input))
        break;
      if(cmdline == NULL)
        break;
      parse(cmdline);
    }
}

enum type_run {
  DEFAULT_RUN,
  SCRIPT,
  COMMAND,
};

void print_help()
{
  fprintf(stderr,"help : minish (run a shell)\n");
  fprintf(stderr,"help : minish -c CMD (run single command)\n");
  fprintf(stderr,"help : minish file_script.minish (run a script)\n");
}
  
int main(int argc, char** argv)
{
  char*  cmdline = NULL;
  int cur = 0;
  int i= 0;
  char* script = NULL;
  char* opt = "c:s:h"; // add d for debug
  enum type_run type_run = DEFAULT_RUN;
  struct option longopt[] = {
    {"command", 1, NULL, 'c'},
    {"script", 1, NULL, 's'},
    {"help", 0, NULL, 'h'},
    //{"debug",0,NULL,'d'}, //uncomment to enable debug option
    {NULL, 0, NULL,'\0'},
  };


  while((cur = getopt_long(argc, argv, opt,longopt, &i)) != -1){
    switch(cur)
      {
      case 'c':
        type_run = COMMAND;
        cmdline = optarg;
        break;
      case 's':
        script = optarg;
        break;
      case 'h':
        print_help();
        exit(EXIT_SUCCESS);
        break;
        /*
          case 'd':
          break;
         */
      case '?':
      default:
      }
  }

  
  {
    addcmd("cd",cd_main);
    addcmd("set",set_main);
  }

  
  {
    int ret = 0;
    FILE* fscript = NULL;
    struct config_prompt cnf_prompt;
    struct config_gets cnf_gets;
    cnf_prompt.cnf_out = DEFAULT_PROMPT;
    if(script != NULL)
      {
        fscript = fopen(script, "rb");
        if(fscript == NULL)
          {
            perror(script);
            exit(EXIT_FAILURE);
          }
        cnf_gets.input = fscript;
        cnf_gets.output = fopen("/dev/null","wb");
        if(cnf_gets.output == NULL)
          {
            perror(script);
            exit(EXIT_FAILURE);
          }

        cnf_prompt.cnf_out = NONE;
      }
    else
      {
        cnf_gets.input = stdin;
      }


    
    interpreter(&cnf_prompt, &cnf_gets);
    if(fscript != NULL)
      {
        fclose(fscript);
      }

    freecmd();
    return ret;
  }
}
