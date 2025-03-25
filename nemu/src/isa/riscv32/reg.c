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

#define COLOR_PC    "\033[1;33m"  // 黄色加粗（程序计数器）
#define COLOR_SPEC  "\033[1;34m"  // 蓝色加粗（sp/gp/tp等特殊寄存器）
#define COLOR_ARG   "\033[32m"    // 绿色（函数参数寄存器a0-a7）
#define COLOR_SAVE  "\033[36m"    // 青色（保存寄存器s0-s11）
#define COLOR_TEMP  "\033[35m"    // 洋红色（临时寄存器t0-t6）
#define COLOR_RESET "\033[0m"     // 重置颜色
#define COLOR_HEX		"\033[38;5;33m" // 亮蓝色 (16进制)
#define COLOR_DEC  "\033[38;5;118m" // 亮绿色 (10进制)
#include <isa.h>
#include "local-include/reg.h"
#include <ctype.h>

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
 // PC寄存器特殊高亮
  printf(COLOR_PC "PC" COLOR_RESET "\t0x%08x\t%u\n", cpu.pc, cpu.pc);

  for (int i = 0; i < (sizeof(regs)/sizeof(regs[0])); i++) {
    const char* color = COLOR_RESET;
    const char* name = regs[i];
    
    // 按寄存器类型设置颜色
    if (strcmp(name, "sp") == 0 || strcmp(name, "gp") == 0 || strcmp(name, "tp") == 0) {
      color = COLOR_SPEC;
    } else if (name[0] == 'a' && isdigit(name[1])) { // a0-a7
      color = COLOR_ARG;
    } else if (name[0] == 's' && isdigit(name[1])) { // s0-s11
      color = COLOR_SAVE;
    } else if (name[0] == 't' && isdigit(name[1])) { // t0-t6
      color = COLOR_TEMP;
    }

    printf("%s%s" COLOR_RESET "\t" COLOR_HEX "0x%08x" COLOR_RESET "\t" COLOR_DEC "%u\n" COLOR_RESET, color, name, cpu.gpr[i], cpu.gpr[i]);
  }
}

word_t isa_reg_str2val(const char *s, bool *success) {
	const char *name = s;
	if (name[0] == '$') name++;

	for (int i = 0; i < (sizeof(regs)/sizeof(regs[0])); i++) {
		if (strcmp(name, regs[i] + 1) == 0) {  // 比较寄存器名，跳过$前缀
			*success = true;
			return cpu.gpr[i];
		}
	}

	*success = false;
  return 0;
}
