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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <limits.h>

enum {
	TK_NOTYPE = 256,
	TK_EQ,
	TK_ADD,
	TK_SUB,
	TK_MUL,
	TK_DIV,
	TK_LPAREN,
	TK_RPAREN,
	TK_NUM

	/* TODO: Add more token types */

};

static struct rule {
	const char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +", TK_NOTYPE},					// spaces
	{"\\+", TK_ADD},						// plus
	{"\\-", TK_SUB},						// minus
	{"\\*", TK_MUL},						// multiply
	{"\\/", TK_DIV},						// divide
	{"\\(", TK_LPAREN},					// left parentheses
	{"\\)", TK_RPAREN},					// right parentheses
	{"[0-9]+", TK_NUM},					// number
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;


static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while (e[position] != '\0') {
		/* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i ++) {
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
						i, rules[i].regex, position, substr_len, substr_len, substr_start);

				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch (rules[i].token_type) {
					case TK_NOTYPE: // 空格直接跳过
						break;
					case TK_NUM:    // 数字需要记录值
					case TK_ADD:
					case TK_SUB:
					case TK_MUL:
					case TK_DIV:
					case TK_LPAREN:
					case TK_RPAREN:
						// 记录token
						if (nr_token >= 32) {
							printf("Error: Too many tokens\n");
							return false;
						}
						tokens[nr_token].type = rules[i].token_type;
						// 对数字复制字符串
						if (rules[i].token_type == TK_NUM) {
							strncpy(tokens[nr_token].str, substr_start, substr_len);
							tokens[nr_token].str[substr_len] = '\0';
						}
						nr_token++;
						break;
					default:
						printf("Error: Unhandled token type %d\n", rules[i].token_type);
						return false;

						// default: TODO();
				}

				break;
			}
		}

		if (i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}

word_t expr(char *e, bool *success);
uint32_t eval(int p, int q, bool *success);
bool check_parentheses(int p, int q);
int find_main_op(int p, int q);
static int get_priority(int token_type);

word_t expr(char *e, bool *success) {
	if (!make_token(e)) {
		*success = false;
		return 0;
	}

	return eval(0, nr_token - 1, success);
	/* TODO: Insert codes to evaluate the expression. */
	// TODO();

}
uint32_t eval(int p, int q, bool *success) {
	printf("eval(%d, %d)\n", p, q);

	if (p > q) {
		*success = false;
		return 0;
	}

	if (p == q) {
		if (tokens[p].type != TK_NUM) {
			*success = false;
			return 0;
		}
		return strtoul(tokens[p].str, NULL, 10);
	} 

	if (check_parentheses(p, q)) {
		return eval(p + 1, q - 1, success);
	}

	int op_pos = find_main_op(p, q);
	if (op_pos == -1) {
		*success = false;
		return 0;
	}

	uint32_t val1 = eval(p, op_pos - 1, success);
	uint32_t val2 = eval(op_pos + 1, q, success);

	switch (tokens[op_pos].type) {
		case TK_ADD: return val1 + val2;
		case TK_SUB: return val1 - val2;
		case TK_MUL: return val1 * val2;
		case TK_DIV: 
								 if (val2 == 0) {
									 *success = false;
									 printf("Error: Division by zero\n");
									 return 0;
								 }
								 return val1 / val2;
		default: 
								 *success = false;
								 return 0;
	}
}


//int find_main_op(int p, int q) {
//    int main_op = -1;
//    int min_priority = INT_MAX;
//    int bracket_count = 0;
//
//    for (int i = p; i <= q; i++) {
//        Token token = tokens[i];
//
//        if (token.type == TK_LPAREN) {
//            bracket_count++;
//        } else if (token.type == TK_RPAREN) {
//            bracket_count--;
//        }
//
//        if (token.type == TK_LPAREN || token.type == TK_RPAREN) {
//            continue;
//        }
//
//        if (bracket_count == 0) {
//            if (token.type == '+' || token.type == '-' || token.type == '*' || token.type == '/') {
//                int priority = (token.type == '+' || token.type == '-') ? 1 : 2;
//                if (priority < min_priority || (priority == min_priority && i < main_op)) {
//                    min_priority = priority;
//                    main_op = i;
//                }
//            }
//        }
//    }
//    return main_op;
//}
//

int find_main_op(int p, int q) {
	int main_op = -1;
	int min_priority = INT_MAX;
	int bracket_level = 0;

	// 从右向左扫描，以处理左结合性
	// 有意思哈!?
	for (int i = q; i >= p; i--) {
		Token token = tokens[i];

		// 处理括号层级
		if (token.type == TK_RPAREN) {
			bracket_level++;
		} else if (token.type == TK_LPAREN) {
			bracket_level--;
			if (bracket_level < 0) {
				return -1; // 括号不匹配，上层处理
			}
		}

		// 仅在括号外层考虑运算符
		if (bracket_level != 0) continue;

		// 获取当前运算符优先级
		int priority = get_priority(token.type);
		if (priority == -1) continue; // 非运算符跳过

		// 选优先级最低且最右的运算符
		if (priority < min_priority) {
			min_priority = priority;
			main_op = i;
		}
	}

	// 处理特殊情况 检查是否找到有效运算符
	if (main_op == -1 || bracket_level != 0) {
		return -1;
	}

	return main_op;
}

bool check_parentheses(int p, int q) {
	if (tokens[p].type != TK_LPAREN || tokens[q].type != TK_RPAREN) {
		return false;
	}

	int count = 0;
	for (int i = p; i <= q; i++) {
		if (tokens[i].type == TK_LPAREN) {
			count++;
		} else if (tokens[i].type == TK_RPAREN) {
			count--;
			// 如果括号不匹配
			if (count < 0) {
				return false;
			}
			// 如果在外层括号闭合前就count=0
			if (count == 0 && i != q) {
				return false;
			}
		}
	}
	return count == 0;
}

static int get_priority(int token_type) {
	switch (token_type) {
		case TK_ADD:    return 1;   // 加减
		case TK_SUB:    return 1;
		case TK_MUL:    return 2;   // 乘除
		case TK_DIV:    return 2;
		default:        return -1;  // 非运算符
	}
}

