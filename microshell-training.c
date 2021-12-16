/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell-training.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: juligonz <juligonz@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/16 22:11:47 by juligonz          #+#    #+#             */
/*   Updated: 2021/12/16 23:23:34 by juligonz         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>

char *const*g_env = NULL;

typedef struct s_cmd
{
	char **args;
	struct s_cmd *next;
	struct s_cmd *pipe;
} t_cmd;

size_t ft_strlen(char const *str)
{
	size_t res = 0;
	while (str[res])
		res++;
	return res;
}

void builtin_cd(t_cmd *cmd)
{
	int nb_args = 0;
	const char *err_bad_args = "error: cd: bad arguments\n";
	const char *err_bad_dir = "error: cd: cannot change directory to ";

	while (cmd->args[nb_args])
		nb_args++;
	if (nb_args != 2)
		write(STDERR_FILENO, err_bad_args, ft_strlen(err_bad_args));
	else if (chdir(cmd->args[1]) == -1)
	{
		write(STDERR_FILENO, err_bad_dir, ft_strlen(err_bad_dir));
		write(STDERR_FILENO, cmd->args[1], ft_strlen(cmd->args[1]));
		write(STDERR_FILENO, "\n", 1);		
	}
}

void exit_fatal_error()
{
	const char *err_fatal = "error: fatal\n";
	write(STDERR_FILENO, err_fatal, ft_strlen(err_fatal));
	exit(1);
}

void error_exec(char *exec)
{
	const char *err_fatal = "error: cannot execute ";
	write(STDERR_FILENO, err_fatal, ft_strlen(err_fatal));
	write(STDERR_FILENO, exec, ft_strlen(exec));
	write(STDERR_FILENO, "\n", 1);
}

char *ft_strdup(char *s)
{
	char *res;

	res = malloc(ft_strlen(s));
	if (res == NULL)
		exit_fatal_error();
	size_t i = -1;
	while(s[++i])
		res[i] = s[i];
	res[i] = 0;
	return res;
}

t_cmd *malloc_cmd(char **args)
{
	t_cmd *result;

	result = malloc(sizeof(t_cmd));
	if (!result)
		exit_fatal_error();
	result->args = args;	
	result->next = 0;
	result->pipe = 0;
	return result;
}

char **avdup(char **av, int idx_start, int idx_end)
{
	char **res;

	res = malloc(sizeof(char*) * (1 + idx_end - idx_start));
	if (res == NULL)
		exit_fatal_error();
	int i = 0;
	while (idx_start < idx_end)
	{
		res[i] = ft_strdup(av[idx_start]);
		i++;
		idx_start++;
	}
	res[i] = 0;
	return res;
}

int get_last_arg_idx(int idx, int ac, char**av)
{
	int i = idx;

	while (i < ac && strcmp(av[i], "|") && strcmp(av[i], ";"))
		i++;
	return i;
}

t_cmd* parse_args(int ac, char**av)
{
	t_cmd *result = NULL;
	t_cmd *iterator = NULL;
	int i = 1;

	while (i < ac)
	{
		while (!strcmp(av[i], ";"))
			if (!(i < ac))
				return result;
		int idx_end = get_last_arg_idx(i, ac, av);

		t_cmd *tmp = malloc_cmd(avdup(av, i, idx_end));
		if (tmp == NULL)
			exit_fatal_error();
		if (result == NULL)
			result = iterator = tmp;
		else if (!strcmp(av[i - 1], "|"))
			iterator = iterator->pipe = tmp;
		else if (!strcmp(av[i - 1], ";"))
			iterator = iterator->next = tmp;
		else
			exit(42);
		i = idx_end + 1;
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
			while (it->pipe)
				it = it->pipe;
		}
		it = it->next;
	}
	
}

int main(int ac, char **av, char **env)
{
	g_env = env;
	t_cmd *res = parse_args(ac, av);
	exec_cmds(res);
	exit(0);
}
// no leaks don't worry :-*
