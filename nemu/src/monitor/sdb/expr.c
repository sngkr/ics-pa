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
  // ( 40
  // ) 41
  // * 42
  // + 43
  // - 45
  // / 47
  LEFT = 40,
  RIGHT = 41,
  MULTI = 42,
  ADD = 43,
  SUB = 45,
  DIV = 47,
  NUM,

  TK_NOTYPE = 256,
  // 257
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

    // 识别任意长度空格
    {" +", TK_NOTYPE},  // spaces
    // '\\' 在c中识别为 '\'
    // '\+' 在正则中表示
    {"\\+", ADD},  // add
    {"\\-", SUB},
    {"\\*", MULTI},
    {"\\/", DIV},
    {"\\(", '('},
    {"\\)", ')'},
    {"[0-9]+", NUM},
    {"==", TK_EQ},  // equal

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
    // 把我们自己订的规则rules存入re数组
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    Log("rules[%d] = %s", i, rules[i].regex);
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
      /*
      compiled：一个指向已经编译的正则表达式的 regex_t 结构体的指针。
      string：要在其中执行匹配的输入字符串。
      nmatch：regmatch_t 结构体数组 pmatch[]
      的大小，即最多可以存储多少个匹配结果。 pmatch[]：一个 regmatch_t
      结构体数组，用于存储匹配结果的位置信息。
      eflags：一个整数，用于指定匹配标志，如 REG_EXTENDED、REG_ICASE 等

      int regexec(const regex_t *preg, const char *string, size_t
      nmatch,regmatch_t pmatch[], int eflags);
      参数preg指向编译后的正则表达式，参数string是将要进行匹配的字符串。
      在匹配结束后，nmatch告诉regexec函数最多可以把多少匹配结果填充到pmatch数组中，
      在匹配结束后，string+pmatch[0].rm_so到string+pmatch[0].rm_eo是第一个匹配的字符串，以此类推。
      */
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        // 把字符串逐个识别成token，存到pmatch
        char *substr_start = e + position;
        // 把token对应的起始字符串地址存入substr_start
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        // Token tmp_token;
        switch (rules[i].token_type) {
          case ADD:
          case SUB:
          case LEFT:
          case RIGHT:
          case DIV:
          case MULTI:
          case NUM:
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token++].str, substr_start, substr_len);
            // Log("Type: %d  ",tokens[nr_token-1].type);
            // int temp = 0;
            // while(temp < substr_len){
            //   printf("%c", tokens[nr_token-1].str[temp++]);
            // }
            // printf("\n");
            break;
          case TK_NOTYPE:
            break;
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

// 检查左右括号是否匹配
int check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int count = 0;
  for (int i = p + 1; i < q; i++) {
    if (tokens[i].type == '(') {
      count++;
    } else if (tokens[i].type == ')') {
      if (count == 0) {
        return false;  // 出现多余的右括号，不匹配
      }
      count--;
    }
  }

  return count == 0;  // 如果 count 不为 0，则有多余的左括号，不匹配
}

int calc(int p, int q) {
  // int sign = 0;
  int count = 0;
  int op = -1;
  for (int i = p; i <= q; i++) {
    // if (tokens[i].type == '(') {
    //   count++;
    //   continue;
    // }
    // if (tokens[i].type == ')') {
    //   count--;
    //   continue;
    // }
    // if (count != 0) {
    //   continue;
    // }
    // if (tokens[i].type == NUM) {
    //   continue;
    // }
    // if (sign <= 1 && (tokens[i].type == '+' || tokens[i].type == '-')) {
    //   op = i;
    //   sign = 1;
    // } else if (sign == 0 && (tokens[i].type == '*' || tokens[i].type == '/'))
    // {
    //   op = i;
    // }
    int precedence = 0;
    // 平级先算哪个都行，对于这个  先算优先级低的
    if (tokens[i].type == LEFT) {
      count++;
    } else if (tokens[i].type == RIGHT) {
      count--;
    } else if (count == 0) {
      int current_precedence;
      switch (tokens[i].type) {
        case ADD:
          current_precedence = 1;
          break;
        case SUB:
          current_precedence = 1;
          break;
        case MULTI:
          current_precedence = 2;
          break;
        case DIV:
          current_precedence = 2;
          break;
        default:
          continue;  // If it's not an operator, we skip it
      }
      if (current_precedence <= precedence) {
        op = i;
        Log("nowop %d", op);
        precedence = current_precedence;
      }
      op = i;
    }
  }
  return op;
}

int eval(int p, int q) {
  if (p > q) {
    /* Bad expression */
    panic("Bad expression");
  } else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    return atoi(tokens[p].str);
  } else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  } else {
    // op = the position of 主运算符 in the token expression;
    int op = calc(p, q);
    Log("op: %d", op);
    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);

    switch (tokens[op].type) {
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*': /* ... */
        return val1 * val2;
      case '/': /* ... */
        return val1 / val2;
      default:
        assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  return eval(0, nr_token - 1);
}
