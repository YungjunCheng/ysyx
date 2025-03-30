/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <errno.h>
#include <limits.h>
#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"


static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
	nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Single-step execute N instructions (default 1)", cmd_si },                                                                                                             
  { "info", "Print program status: 'info r' for registers, 'info w' for watchpoints", cmd_info },
  { "x", "Print N 4-byte values starting at address EXPR", cmd_x },
  { "p", "Evaluate and print the value of expression EXPR", cmd_p },
  { "w", "Set a watchpoint", cmd_w },
  { "d", "Delete a watchpoit with the given number", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Error: Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
	char *arg = strtok(NULL , " ");
  int steps = 1;  // 默认步数为1

  if (arg) {
    char *endptr;
    errno = 0;
    long val = strtol(arg, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || val <= 0 || val > INT_MAX) {
      printf("Error: Invalid argument '%s'. Usage: si [N]\n", arg);
        return 0;
      }
      steps = (int)val;
    }

    if (strtok(NULL, " ") != NULL) {
        printf("Error: Extra argument. Usage: si [N]\n");
        return 0;
    }

  cpu_exec(steps);
  return 0;
}

static int cmd_info(char *args) {
	char *arg = strtok(args, " ");

	if (arg == NULL) {
    printf("Error: Missing subcommand. Usage: info w or info r\n");
    return 0;
  }

	 if (strtok(NULL, " ") != NULL) {
    printf("Error: Too many arguments. Usage: info w or info r\n");
    return 0;
  }

	if (strcmp(arg, "r") == 0) {
    isa_reg_display();
    return 0;
  }
  else if (strcmp(arg, "w") == 0) {
    // TODO: 实现watchpoint功能
    return 0;
  }
  else {
    printf("Error: Invalid subcommand '%s'. Valid options: w, r\n", arg);
    return 0;
  }
	
	return 0;
}

word_t vaddr_read(vaddr_t, int);

static int cmd_x(char *args) {
	 char *arg1 = strtok(NULL, " ");
   char *arg2 = strtok(NULL, " ");


	if (arg1 == NULL ||  arg2 == NULL) {
		printf("Error: Missing arguments. Usage: x [N] EXPR\n");
		return 0;
	}
	
	if (strtok(NULL, " ") != NULL) {
        printf("Error: Too many arguments. Usage: x [N] EXPR\n");
        return 0;
    }
	
	char *endptr;
  errno = 0;
  long count = strtol(arg1, &endptr, 10);

  if (errno != 0 || *endptr != '\0' || count <= 0 || count > INT_MAX) {
    printf("Error: Invalid count '%s'. Must be a positive integer\n", arg1);
    return 0;
  }

	errno = 0;
  vaddr_t addr = strtol(arg2, &endptr, 16);

  if (errno != 0 || *endptr != '\0') {
    printf("Error: Invalid address '%s'. Must be a hexadecimal number\n", arg2);
    return 0;
  }

  for (int i = 0; i < count; i++) {
    if (i % 4 == 0) {
        printf("\033[34m0x%08x\033[0m: ", addr + i * 4);
    }
    uint32_t data = vaddr_read(addr + i * 4, 4);
    printf("0x%08x ", data);
    if (i % 4 == 3) {
      printf("\n");
    }
  }

  if (count % 4 != 0) {
    printf("\n");
  }
  return 0;
}

static int cmd_p(char *args) {
 if (args == NULL || *args == '\0') {  // 检查是否有输入表达式
    printf("Error: Missing expression. Usage: p EXPR\n");
    return 0;
  }
	
  bool success = true;
  word_t result = expr(args, &success); // 调用表达式求值函数

  if (success) {
    printf("%u\n", result); // 输出十进制结果
    printf("%u (0x%08x)\n", result, result);	// 输出十六进制结果
  } else {
    printf("Error: Invalid expression '%s'\n", args);
  }
	
  return 0;
}

static int cmd_w(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_d(char *args) {
  cpu_exec(-1);
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Error: Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
