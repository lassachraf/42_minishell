/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alassiqu <alassiqu@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/04 20:58:27 by alassiqu          #+#    #+#             */
/*   Updated: 2024/06/30 17:43:51 by alassiqu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "includes/minishell.h"

t_minishell	*g_minishell;

void	print_root(t_type type, int x)
{
	if (type == PIPE)
	{
		if (x == 1)
			printf("\nLEFT OF '|' -> ");
		else if (x == 0)
			printf("\nRIGHT OF '|' ----> ");
	}
	else if (type == OR)
	{
		if (x == 1)
			printf("\nLEFT OF '||' ----> ");
		else if (x == 0)
			printf("\nRIGHT OF '||' ----> ");
	}
	else if (type == AND)
	{
		if (x == 1)
			printf("\nLEFT OF '&&' ----> ");
		else if (x == 0)
			printf("\nRIGHT OF '&&' ----> ");
	}
	else if (type == L_PAREN)
	{
		if (x == 1)
			printf("\nLEFT OF '(' ----> ");
		else if (x == 0)
			printf("\nRIGHT OF '(' ----> ");
	}
}

void	printAST(t_node *node, int x, t_type type)
{
	t_type	tmp;
	t_redir	*new;

	if (!node)
		return ;
	tmp = ERROR;
	if (node->type == STRING_NODE)
	{
		print_root(type, x);
		while (node->data.cmd)
		{
			printf("'%s' ", (char *)node->data.cmd->content);
			node->data.cmd = node->data.cmd->next;
		}
		printf("\n");
	}
	else if (node->type == PAIR_NODE)
	{
		print_root(type, x);
		if (node->data.pair.type == PIPE)
		{
			printf("\n*** PIPE ***\n");
			tmp = PIPE;
		}
		else if (node->data.pair.type == OR)
		{
			printf("\n*** OR ***\n");
			tmp = OR;
		}
		else if (node->data.pair.type == AND)
		{
			printf("\n*** AND ***\n");
			tmp = AND;
		}
		else if (node->data.pair.type == L_PAREN)
		{
			printf("L_PAREN\n");
			tmp = L_PAREN;
		}
		// if(node->data.pair.type <= 3)
		// {
		printAST(node->data.pair.left, 1, tmp);
		printAST(node->data.pair.right, 0, tmp);
		// }
		// else
		// {
		//     printAST(node->data.pair.left, 1 , tmp);
		//     printAST(node->data.pair.right, 0 , tmp);
		// }
	}
	else if (node->type == REDIR_NODE)
	{
		print_root(type, x);
		while (node->data.redir)
		{
			new = node->data.redir->content;
			printf("REDIR NODE , name: '%s'\n", new->file);
			while (new->cmd)
			{
				printf("'%s' ", (char *)new->cmd->content);
				new->cmd = new->cmd->next;
			}
			printf("\n");
			node->data.redir = node->data.redir->next;
		}
	}
	else if (node->type == ERROR_NODE)
	{
		printf("add '%p', -ERROR -------> '%s", node, node->data.error);
	}
}

void	print_tokens(t_token *tokens)
{
	t_token	*token;

	token = tokens;
	while (token)
	{
		printf("value => '%s'\n", token->value);
		printf("type => '%u'\n", token->type);
		token = token->next;
	}
}

void	increment_shlvl()
{
	char	*shlvl;
	char	*new_shlvl;
	int		tmp;

	shlvl = get_env_var(g_minishell->our_env, "SHLVL");
	tmp = ft_atoi(shlvl) + 1;
	new_shlvl = ft_itoa(tmp);
	gc_add(g_minishell, new_shlvl);
	set_env_var(g_minishell->our_env, "SHLVL", new_shlvl, 1);
}

int	init_minishell(char **env)
{
	g_minishell = malloc(sizeof(t_minishell));
	if (!g_minishell)
		return (0);
	g_minishell->dq_flag = 0;
	g_minishell->gc = NULL;
	g_minishell->stdin = dup(0);
	g_minishell->stdout = dup(1); 
	g_minishell->our_env = dup_env(env);
	if (!g_minishell->our_env)
		return (print_errors("dup_env failed !"), 0);
	increment_shlvl();
	add_env_var(g_minishell->our_env, "?", "0", false);
	signals();
	return (1);
}

int	get_exit_status()
{
	char	*value;
	int		exit_status;

	value = get_env_var(g_minishell->our_env, "?");
	exit_status = ft_atoi(value);
	return (exit_status);
}

void	ft_readline()
{
	int	exit_status;
	
	g_minishell->line = readline(ORANGE PROMPT RESET);
	gc_add(g_minishell, g_minishell->line);
	set_env_var(g_minishell->our_env, "?", "0", 0);
	if (!g_minishell->line)
	{
		exit_status = get_exit_status();
		ft_putstr_fd("exit\n", 1);
		clear_env(g_minishell->our_env);
		gc_free_all(g_minishell);
		free(g_minishell);
		exit(exit_status);
	}
	if (g_minishell->line[0])
		add_history(g_minishell->line);
}

int	main(int ac, char **av, char **env)
{
	(void)ac, (void)av;
	if (!init_minishell(env))
		return (1);
	while (1)
	{
		ft_readline();
		g_minishell->tokens = tokenizer();
		if (!g_minishell->tokens || syntax() == -1)
			continue ;
		// tokens and syntax are totally good in terms of leaks.
		// chof m3aya hdshi tl parsing wl expand bash ma nzgel walo.
		g_minishell->ast = parsing();
		if (!g_minishell->ast)
			continue ;
		// printAST(g_minishell->ast, 700, 403298);
		executer(g_minishell->ast);
		dup2(g_minishell->stdin, 0);
		while(waitpid(-1, NULL, 0) != -1);
		gc_free_all(g_minishell);
	}
	gc_free_all(g_minishell);
	clear_env(g_minishell->our_env);
	free(g_minishell);
	return (0);
}
