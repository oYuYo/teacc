#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//トークンの種類
typedef enum{
    TK_RESERVED,    //記号
    TK_NUM,         //整数トークン
    TK_EOF,          //入力の終わりを表すトークン
} TokenKind;
typedef struct Token Token; //Token構造体をTokenとして定義
struct Token{
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMの場合、その数値
    char *str;      //トークン文字列
    int len;        //トークンの長さ
};
Token *token;       //現在注目しているトークン

//抽象構文木のノードの種類
typedef enum{
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LT,      // <
    ND_LE,      // <=
    ND_NUM,     // num
} NodeKind;
//抽象構文木のノードの型
typedef struct Node Node;
struct Node{
    NodeKind kind;  // ノードの型
    Node *lhs;      // 左辺(left-hand side)
    Node *rhs;      // 右辺(right-hand side)
    int val;        // kindがND_NUMの場合のみ使用
};

char *user_input;

//***プロトタイプ宣言***
bool consume(char *);
void expect(char *);
int expect_number();
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
//*********************

Node *new_node(NodeKind kind, Node *lhs, Node *rhs){
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val){
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *expr(){           //EBNF expr = equality
    return equality();
}

Node *equality(){       //EBNF equality = relational ("==" relational | "!=" relational)*
    Node *node = relational();

    for(;;){
        if(consume("=="))
            node = new_node(ND_EQ, node, relational()); //**new_binary
        else if(consume("!="))
            node = new_node(ND_NE, node, relational()); //**
        else
            return node;
    }
}

Node *relational(){     //EBNF relational = add ("<" add | "<=" add | ">" add | ">=" add)*
    Node *node = add();

    for(;;){
        if(consume("<"))
            node = new_node(ND_LT, node, add());
        else if(consume("<="))
            node = new_node(ND_LE, node, add());
        else if(consume(">"))                           //両辺を入れ替える
            node = new_node(ND_LT, add(), node);
        else if(consume(">="))                          //両辺を入れ替える
            node = new_node(ND_LE, add(), node);
        else
            return node;
    }
}

Node *add(){            //EBNF add = mul ("+" mul | "-" mul)*
    Node *node = mul();

    for(;;){
        if(consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if(consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul(){            //EBNF mul = unary ("*" unary | "/" unary)*
    Node *node = unary();

    for(;;){
        if(consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if(consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary(){          //EBNF unary = ("+" | "-")? primary
    if(consume("+"))
        return primary();
    if(consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());

    return primary();
}

Node *primary(){        //EBNF primary = num | "(" expr ")"

    //次のトークンが"("ならば、"(" expr ")"のはずである
    if(consume("(")){
        Node *node = expr();
        expect(")");
        return node;
    }

    //そうでなければ数値
    return new_node_num(expect_number());
}

//エラーのある場所を表示する
void error_at(char *loc, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);  //ポインタをセット

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");   //pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error(char *fmt, ...){     //"..." : 動的引数
    va_list ap;                 //可変引数が格納されるva_list型を定義
    va_start(ap, fmt);          //可変引数のポインタをセット
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号のときには、トークンを1つ読み進めて真を返す
//それ以外の場合には偽を返す。
bool consume(char *op){
    if(token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

//次のトークンが期待している記号のときには、トークンを1つ読み進める
//それ以外の場合にはエラーを報告する
void expect(char *op){
    if(token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        error("'%c'ではありません", op);
    token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す
//それ以外の場合にはエラーを報告する
int expect_number(){
    if(token->kind != TK_NUM)
        error_at(token->str, "数値ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(){
    return token->kind == TK_EOF;
}

bool startwith(char *p, char *q){
    return memcmp(p, q, strlen(q)) == 0;    //byte(p) ＝ byte(q) ?
}

//新しいトークンを作成してcurにつなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len){
    Token *tok = calloc(1, sizeof(Token));  //heapからTokenサイズのバイトを1個確保し、0で初期化
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

//入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p){
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while(*p){
        //空白文字をスキップ
        if(isspace(*p)){
            p++;
            continue;
        }
        //2文字の演算子を検索
        if(startwith(p, "==") || startwith(p, "!=") || startwith(p, "<=") || startwith(p, ">=")){
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        //1文字の演算子を検索
        if(strchr("+-*/()<>", *p)){
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }
        if(isdigit(*p)){
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);   //10進のlong型へ変換
            cur->len = p - q;               //??
            continue;
        }
        error("トークナイズできません\n");
    }
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

//generator
void gen(Node *node){
    if(node->kind == ND_NUM){
        printf("  push %d\n", node->val);
        return;
    }
    
    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch(node->kind){
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }
    printf("  push rax\n");
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    //トークナイズする
    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //抽象構文木を下りながらコード生成
    gen(node);

    //スタックトップに式全体の値が残っているはずなので
    //それをRAXにロードして関数からの返り値とする
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}