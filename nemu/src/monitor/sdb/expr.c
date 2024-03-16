/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  // + 43 
  // - 45
  // * 42
  // / 47
  MULTI = 41,
  ADD = 43,
  SUB = 45,
  DIV = 47,
  NUM,

  TK_NOTYPE = 256, 
  //257
  TK_EQ,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    //识别任意长度空格
    {" +", TK_NOTYPE},    // spaces
    // '\\' 在c中识别为 '\'
    // '\+' 在正则中表示
    {"\\+", ADD},        // add
    {"\\-", SUB},         
    {"\\*", MULTI},         
    {"\\/", DIV}, 
    {"[0-9]+", NUM},        
    {"==", TK_EQ},        // equal



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

  for (i = 0; i < NR_REGEX; i++) {
    /*
  函数说明：Regcomp将正则表达式字符串regex编译成regex_t的形式，后续regexec以此进行搜索。
  参数说明：
      Preg：一个regex_t结构体指针。
      Regex：正则表达式字符串。
      Cflags：是下边四个值或者是他们的或(|)运算。
          REG_EXTENDED：使用POSIX扩展正则表达式语法解释的正则表达式。如果没有设置，基本POSIX正则表达式语法。
          REG_ICASE：忽略字母的大小写。
          REG_NOSUB：不存储匹配的结果。
          REG_NEWLINE：对换行符进行“特殊照顾”，后边详细说明。
    返回值：
       0：表示成功编译；
      非0：表示编译失败，用regerror查看失败信息
    */

    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    Log("match rules[%d] = \"%s\" ", i ,rules[i].regex);
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
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        // Token tmp_token;
        // switch (rules[i].token_type) {
        //   default: TODO();
        // }
        // len = nr_token;
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

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();

  return 0;
}
