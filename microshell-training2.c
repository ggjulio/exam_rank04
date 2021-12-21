
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <sys/types.h>
#include <sys/wait.h>

char ** g_env = NULL;

typedef struct s_cmd
{
  char **args;
  struct s_cmd *pipe;
  struct s_cmd *next;
} t_cmd;

size_t ft_strlen(char const *s)
{
  size_t i = 0;
  while (s[i])
    i++;
  return i;
}

void builtin_cd(t_cmd *cmd)
{
  const char* err_bad_args = "error: cd: bad arguments\n";
  const char* err_cant_cd = "error: cd: cannot change directory to ";

  int ac = 0;
  while (cmd->args[ac])
    ac++;

  if (ac != 2)
    write(STDERR_FILENO, err_bad_args, ft_strlen(err_bad_args));
  else if (chdir(cmd->args[1]) == -1)
    {
      write(STDERR_FILENO, err_cant_cd, ft_strlen(err_cant_cd));
      write(STDERR_FILENO, cmd->args[1], ft_strlen(cmd->args[1]));
      write(STDERR_FILENO, "\n", 1);
    }
}

void exit_fatal_error(void)
{
  const char* err = "error: fatal\n";

  write(STDERR_FILENO, err, ft_strlen(err));
  exit(1);
}

void error_exec(char *exec)
{
  const char* err = "error: cannot execute ";

  write(STDERR_FILENO, err, ft_strlen(err));
  write(STDERR_FILENO, exec, ft_strlen(exec));
  write(STDERR_FILENO, "\n", 1);
}

t_cmd *malloc_cmd(char **args)
{
  t_cmd *result = malloc(sizeof(t_cmd));
  if (result == NULL)
    exit_fatal_error();
  result->args = args;
  result->pipe = 0;
  result->next = 0;
  return result;
}

char* ft_strdup(const char* s)
{
  char* res = malloc(ft_strlen(s) + 1);

  if (res == NULL)
    exit_fatal_error();
  int i = -1;
  while (s[++i])
    res[i] = s[i];
  res[i] = 0;
  return res;
}

char ** avdup(char **av, int start, int end)
{
  char **res = malloc(sizeof(char*) * (1 + end - start));
  if (res == NULL)
    exit_fatal_error();
  int i = 0;
  while (start < end)
    {
      res[i] = ft_strdup(av[start]);
      start++;
      i++;
    }
  res[i] = 0;
  return res;
}

int get_last_arg_idx(int i, int ac, char** av)
{
  while (i < ac && strcmp(av[i], ";") && strcmp(av[i], "|"))
    i++;
  return i;
}

t_cmd *parse_args(int ac, char ** av)
{
  t_cmd *result = NULL;
  t_cmd *iterator = NULL;
  int i = 1;

  while (i < ac)
    {
      while (!strcmp(av[i], ";"))
	if (!(++i < ac))
	  return result;
      int end_arg = get_last_arg_idx(i, ac, av);

      t_cmd *tmp = malloc_cmd(avdup(av, i, end_arg));
      if (result == NULL)
	result = iterator = tmp;
      else if (!strcmp(av[i - 1], "|"))
	iterator = iterator->pipe = tmp;
      else if (!strcmp(av[i - 1], ";"))
	iterator = iterator->next = tmp;
      else
	exit(42);
      i = end_arg + 1;
    }
  return result;
}

pid_t spawn(int in, int out, t_cmd *cmd)
{
  pid_t pid;

  if ((pid = fork()) == 0)
    {
      if (in != 0)
	{
	  dup2(in, 0);
	  close(in);
	}
      if (out != 1)
	{
	  dup2(out, 1);
	  close(out);
	}
      if (execve(cmd->args[0], cmd->args, g_env) == -1)
	error_exec(cmd->args[0]);
    }
  if (pid == -1)
    exit_fatal_error();
  return pid;
}

void exec_pipeline(t_cmd *it)
{
  int in, fds[2];

  in = 0;
  while (it->pipe)
    {
      if (pipe(fds) == -1)
	exit_fatal_error();
      spawn(in, fds[1], it);
      close(fds[1]);
      if (in && it->pipe)
      	close(in);
      in = fds[0];
      it = it->pipe;
    }
  spawn(in, 1, it);
  if (in != 0)
    close(in);
}

void exec_cmds(t_cmd *it)
{
  int status;
  while (it)
    {
      if (!strcmp(it->args[0], "cd"))
	builtin_cd(it);
      else
	{
	  exec_pipeline(it);
	  wait(&status);
	  while(it->pipe)
	    it = it->pipe;
	}
      it = it->next;
    }
}


void free_cmd(t_cmd *to_free)
{
  size_t i = 0;
  while (to_free->args[i])
    free(to_free->args[i++]);
  free(to_free->args);
  free(to_free);
}

void shit(t_cmd *cmd)
{
  t_cmd *tmp = cmd->next ? cmd->next : cmd->pipe;
  free_cmd(cmd);
  if (tmp)
    shit(tmp);
}

int main(int ac, char**av, char **env)
{
  g_env = env;
  t_cmd *cmds = parse_args(ac, av);
  exec_cmds(cmds);
  shit(cmds);
  return 0;
}

// errno == EBADF
