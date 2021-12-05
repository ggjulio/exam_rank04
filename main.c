#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


typedef struct s_cmd{
	char			**args;
	struct s_cmd	*pipe;
	struct s_cmd	*next;
} t_cmd;

int ft_strlen(char const *str)
{
	int i = 0;

	while(str[i])
		i++;
	return (i);
}

void builtin_cd(t_cmd *cmd)
{
	int nb_args = 0;
	const char *err_bad_args = "error: cd: bad arguments\n"; 
	const char *err_cant_cd = "error: cd: cannot change directory to "; 

	while (cmd->args[nb_args])
		nb_args++;
	if (nb_args != 2)
		write(STDERR_FILENO, err_bad_args, ft_strlen(err_bad_args));
	if (chdir(cmd->args[1]))
	{
		write(STDERR_FILENO, err_cant_cd, ft_strlen(err_cant_cd));		
		write(STDERR_FILENO, cmd->args[1], ft_strlen(cmd->args[1]));
		write(STDERR_FILENO, "\n", 1);
	}
}

void exit_fatal_error()
{
	const char *err = "error: fatal\n";

	write(STDERR_FILENO, err, ft_strlen(err));
	exit(1);
}

t_cmd *malloc_cmd(char **args)
{
	t_cmd *result;

	result = malloc(sizeof(t_cmd));
	if (result == NULL)
		return (NULL);
	result->args = args;
	result->pipe = NULL;
	result->next = NULL;
	return result;
}

char *ft_strdup(char *str)
{
	char *res = NULL;

	res = malloc(ft_strlen(str) + 1);
	if (res == NULL)
		return NULL;
	int i = -1;
	while (str[++i])
		res[i] = str[i];
	res[i] = 0;
	return res;
}

char **avdup(char **av, int idx_start, int idx_end)
{
	char **res;

	res = malloc(sizeof(char *) * (1 + idx_end - idx_start));
	if (res == NULL)
		return NULL;
	int i = 0;
	while (idx_start < idx_end)
	{
		res[i] = ft_strdup(av[idx_start]);
		idx_start++;
		i++;
	}
	res[i] = NULL;
	return res;
}

int get_last_args_idx(int idx, int ac, char ** av)
{
	int i = idx;

	while(i < ac && strcmp(av[i], "|") && strcmp(av[i], ";"))
		i++;
	return i;
}

t_cmd *parse_args(int ac, char **av)
{
	t_cmd *result = NULL;
	t_cmd *iterator = NULL;
	int i = 0;
	
	while (i < ac)
	{
		int idx_end = get_last_args_idx(i + 1, ac, av);

		t_cmd *tmp = malloc_cmd(avdup(av, i + 1, idx_end));
		if (tmp == NULL)
			exit_fatal_error();
		else if (result == NULL)
		{
			result = tmp;
			iterator = tmp;
		}
		else
		{
			if (!strcmp(av[i],"|"))
			{
				iterator->pipe = tmp;
				iterator = iterator->pipe;
			}
			else if (!strcmp(av[i], ";"))
			{
				iterator->next = tmp;
				iterator = iterator->next;
			}
		}
		i = idx_end;
	}
	return result;
}

void print_array(char **arr)
{
	int i = 0;
	printf("-------\n");
	while (arr[i])
	{
		printf("    %s\n", arr[i]);
		i++;
	}
}

void print_cmd(t_cmd *cmd)
{
	t_cmd *iterator = cmd;

	while (iterator)
	{
		print_array(iterator->args);

		if (iterator->next != NULL)
			iterator = iterator->next;
		else
			iterator = iterator->pipe;
	}
}

int main(int ac, char **av, char **env)
{
	(void)ac;
	(void)av;
	(void)env;
    t_cmd* res = parse_args(ac, av);
    print_cmd(res);
	return 0;
}
