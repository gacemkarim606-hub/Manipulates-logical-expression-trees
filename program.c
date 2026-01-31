#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXTOK 300
#define MAXLEN 64
#define STACK_CAP 300
#define VAR_CAP 100

// ----------------- Token -----------------
typedef struct {
    char text[MAXLEN];
} Token;

// ----------------- Node -----------------
typedef struct Node {
    char *value;            // strdup'd string
    struct Node *left;
    struct Node *right;
} Node;

// ----------------- Node stack -----------------
typedef struct {
    Node* items[STACK_CAP];
    int top;
} NodeStack;

void pushNode(NodeStack *s, Node *n) { s->items[++s->top] = n; }
Node* popNode(NodeStack *s) { return s->items[s->top--]; }
int nodeStackEmpty(NodeStack *s) { return s->top == -1; }

// ----------------- Op stack for shunting yard -----------------
char opStack[MAXTOK][MAXLEN];
int opTop = -1;

void pushOp(const char *s) { strcpy(opStack[++opTop], s); }
char* popOp() { return opStack[opTop--]; }
char* peekOp() { return (opTop<0) ? NULL : opStack[opTop]; }
int opEmpty() { return opTop == -1; }
void resetOp() { opTop = -1; }

// ----------------- Variable store -----------------
typedef struct {
    char name[MAXLEN];
    char value[MAXLEN];
    int filled;
} Var;

Var varTable[VAR_CAP];
int varCount = 0;

char* lookupVar(const char *name) {
    for (int i = 0; i < varCount; i++)
        if (strcmp(varTable[i].name, name) == 0 && varTable[i].filled)
            return varTable[i].value;
    return NULL;
}

char* askVar(const char *name) {
    char buf[MAXLEN];
    char *existing = lookupVar(name);
    if (existing) return existing;

    printf("Enter value for '%s' (true/false or integer): ", name);
    if (!fgets(buf, sizeof(buf), stdin)) exit(1);

    size_t L = strlen(buf);
    if (L && buf[L-1]=='\n') buf[L-1]=0;

    if (varCount >= VAR_CAP) {
        printf("Too many variables\n");
        exit(1);
    }

    strcpy(varTable[varCount].name, name);
    strcpy(varTable[varCount].value, buf);
    varTable[varCount].filled = 1;

    return varTable[varCount++].value;
}

// ----------------- Utility helpers -----------------
int isLogicalOp(const char *s) {
    return (strcmp(s,"AND")==0) || (strcmp(s,"OR")==0) || (strcmp(s,"NOT")==0);
}

int isRelOp(const char *op) {
    return strcmp(op, "<")  == 0 ||
           strcmp(op, ">")  == 0 ||
           strcmp(op, "<=") == 0 ||
           strcmp(op, ">=") == 0 ||
           strcmp(op, "==") == 0 ||
           strcmp(op, "!=") == 0;
}

int isOperator(const char *op) {
    return isLogicalOp(op) || isRelOp(op);
}

int isBooleanStr(const char *s) {
    return (strcmp(s,"true")==0) || (strcmp(s,"false")==0);
}

int toBool(const char *s) {
    return strcmp(s,"true")==0 ? 1 : 0;
}

char* boolToStr(int b) {
    return b ? "true" : "false";
}

int isNumberStr(const char *s) {
    if (!s || !s[0]) return 0;
    int i = 0;
    if (s[0]=='-' || s[0]=='+') i=1;
    for (; s[i]; i++)
        if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

// ----------------- Tokenizer -----------------
int tokenize(char *expr, Token tokens[]) {
    int count = 0;
    int i = 0, L = strlen(expr);

    while (i < L) {
        if (isspace((unsigned char)expr[i])) { i++; continue; }

        if (expr[i]=='(' || expr[i]==')') {
            tokens[count].text[0] = expr[i];
            tokens[count].text[1] = '\0';
            count++; i++;
            continue;
        }

        if ((expr[i]=='<' || expr[i]=='>' || expr[i]=='!' || expr[i]=='=') && i+1 < L && expr[i+1]=='=') {
            tokens[count].text[0]=expr[i];
            tokens[count].text[1]='=';
            tokens[count].text[2]='\0';
            count++; i+=2;
            continue;
        }

        if (expr[i]=='<' || expr[i]=='>') {
            tokens[count].text[0]=expr[i];
            tokens[count].text[1]='\0';
            count++; i++;
            continue;
        }

        if (isalpha((unsigned char)expr[i]) || expr[i]=='_') {
            int start = i;
            while (i < L && (isalnum((unsigned char)expr[i]) || expr[i]=='_')) i++;
            int len = i - start;
            if (len >= MAXLEN) len = MAXLEN-1;
            strncpy(tokens[count].text, &expr[start], len);
            tokens[count].text[len]='\0';
            count++;
            continue;
        }

        if (isdigit((unsigned char)expr[i]) || ((expr[i]=='+'||expr[i]=='-') && i+1<L && isdigit((unsigned char)expr[i+1]))) {
            int start = i;
            if (expr[i]=='+'||expr[i]=='-') i++;
            while (i<L && isdigit((unsigned char)expr[i])) i++;
            int len = i-start;
            if (len>=MAXLEN) len=MAXLEN-1;
            strncpy(tokens[count].text,&expr[start],len);
            tokens[count].text[len]='\0';
            count++;
            continue;
        }

        printf("Unknown character in tokenizer: '%c'\n", expr[i]);
        exit(1);
    }

    return count;
}

// ----------------- Precedence & shunting-yard -----------------
int precedence(const char *op) {
    if (strcmp(op,"NOT")==0) return 5;
    if (isRelOp(op)) return 4;
    if (strcmp(op,"AND")==0) return 3;
    if (strcmp(op,"OR")==0) return 2;
    return 0;
}

int isRightAssoc(const char *op) {
    return strcmp(op,"NOT")==0;
}

int infixToPostfix(Token tokens[], int n, char output[][MAXLEN]) {
    opTop=-1;
    int k=0;

    for (int i=0;i<n;i++) {
        char *t = tokens[i].text;

        if (strcmp(t,"(")==0) { pushOp(t); continue; }
        if (strcmp(t,")")==0) {
            while (!opEmpty() && strcmp(peekOp(),"(")!=0) strcpy(output[k++], popOp());
            if (opEmpty()) { printf("Mismatched parentheses\n"); exit(1); }
            popOp();
            continue;
        }

        if (isOperator(t)) {
            while (!opEmpty() && isOperator(peekOp())) {
                int p1=precedence(peekOp()), p2=precedence(t);
                if ((p1>p2) || (p1==p2 && !isRightAssoc(t)))
                    strcpy(output[k++], popOp());
                else break;
            }
            pushOp(t);
            continue;
        }

        strcpy(output[k++], t);
    }

    while (!opEmpty()) {
        if (strcmp(peekOp(),"(")==0 || strcmp(peekOp(),")")==0) {
            printf("Mismatched parentheses\n"); exit(1);
        }
        strcpy(output[k++], popOp());
    }

    return k;
}

// ----------------- Build expression tree -----------------
Node* createNodeStr(const char *s) {
    Node *n = malloc(sizeof(Node));
    n->value = strdup(s);
    n->left = n->right = NULL;
    return n;
}

Node* buildExpressionTree(char* postfix[], int n) {
    NodeStack st; st.top=-1;

    for (int i=0;i<n;i++) {
        char *token=postfix[i];
        if (!token) continue;

        if (!isOperator(token))
            pushNode(&st, createNodeStr(token));
        else {
            Node *node = createNodeStr(token);

            if (strcmp(token,"NOT")==0) {
                if(nodeStackEmpty(&st)){ printf("Malformed NOT\n"); exit(1); }
                node->left=popNode(&st);
            } else {
                if(nodeStackEmpty(&st)){ printf("Malformed binop right\n"); exit(1); }
                node->right=popNode(&st);

                if(nodeStackEmpty(&st)){ printf("Malformed binop left\n"); exit(1); }
                node->left=popNode(&st);
            }
            pushNode(&st, node);
        }
    }

    return popNode(&st);
}

// ----------------- Simplify -----------------
Node* simplify(Node *root) {
    if(!root) return NULL;

    root->left=simplify(root->left);
    root->right=simplify(root->right);

    // NOT
    if(strcmp(root->value,"NOT")==0 && root->left && isBooleanStr(root->left->value)){
        int b=toBool(root->left->value);
        free(root->left); root->left=NULL;
        free(root->value); root->value=strdup(boolToStr(!b));
        return root;
    }

    // Relational folding
    if(isRelOp(root->value) && root->left && root->right && isNumberStr(root->left->value) && isNumberStr(root->right->value)){
        int a=atoi(root->left->value), b=atoi(root->right->value), res=0;
        if(strcmp(root->value,"<")==0) res=a<b;
        if(strcmp(root->value,">")==0) res=a>b;
        if(strcmp(root->value,"<=")==0) res=a<=b;
        if(strcmp(root->value,">=")==0) res=a>=b;
        if(strcmp(root->value,"==")==0) res=a==b;
        if(strcmp(root->value,"!=")==0) res=a!=b;
        free(root->left); free(root->right); root->left=root->right=NULL;
        free(root->value); root->value=strdup(res?"true":"false");
        return root;
    }

    // AND/OR simplification
    if(strcmp(root->value,"AND")==0){
        if(root->left && isBooleanStr(root->left->value) && strcmp(root->left->value,"false")==0){ free(root); return createNodeStr("false"); }
        if(root->right && isBooleanStr(root->right->value) && strcmp(root->right->value,"false")==0){ free(root); return createNodeStr("false"); }
        if(root->left && isBooleanStr(root->left->value) && strcmp(root->left->value,"true")==0){ Node *r=root->right; free(root); return r; }
        if(root->right && isBooleanStr(root->right->value) && strcmp(root->right->value,"true")==0){ Node *l=root->left; free(root); return l; }
    }

    if(strcmp(root->value,"OR")==0){
        if(root->left && isBooleanStr(root->left->value) && strcmp(root->left->value,"true")==0){ free(root); return createNodeStr("true"); }
        if(root->right && isBooleanStr(root->right->value) && strcmp(root->right->value,"true")==0){ free(root); return createNodeStr("true"); }
        if(root->left && isBooleanStr(root->left->value) && strcmp(root->left->value,"false")==0){ Node *r=root->right; free(root); return r; }
        if(root->right && isBooleanStr(root->right->value) && strcmp(root->right->value,"false")==0){ Node *l=root->left; free(root); return l; }
    }

    return root;
}

// ----------------- Traversals -----------------
void printInfix(Node *r){
    if(!r) return;
    if(r->left) printf("(");
    printInfix(r->left);
    printf("%s",r->value);
    if(r->right){ printf(" "); printInfix(r->right); }
    if(r->left) printf(")");
}

void printPostfix(Node *r){
    if(!r) return;
    printPostfix(r->left);
    printPostfix(r->right);
    printf("%s ",r->value);
}

// Evaluate
int evaluate(Node *r){
    if(!r) return 0;
    if(isBooleanStr(r->value)) return toBool(r->value);
    if(isNumberStr(r->value)) return atoi(r->value)!=0;
    if(strcmp(r->value,"NOT")==0) return !evaluate(r->left);
    if(strcmp(r->value,"AND")==0) return evaluate(r->left) && evaluate(r->right);
    if(strcmp(r->value,"OR")==0) return evaluate(r->left) || evaluate(r->right);

    if(isRelOp(r->value)){
        int a,b;
        if(isNumberStr(r->left->value)) a=atoi(r->left->value); else a=atoi(askVar(r->left->value));
        if(isNumberStr(r->right->value)) b=atoi(r->right->value); else b=atoi(askVar(r->right->value));

        if(strcmp(r->value,"<")==0) return a<b;
        if(strcmp(r->value,">")==0) return a>b;
        if(strcmp(r->value,"<=")==0) return a<=b;
        if(strcmp(r->value,">=")==0) return a>=b;
        if(strcmp(r->value,"==")==0) return a==b;
        if(strcmp(r->value,"!=")==0) return a!=b;
    }

    char *v = askVar(r->value);
    if(isBooleanStr(v)) return toBool(v);
    if(isNumberStr(v)) return atoi(v)!=0;

    printf("Cannot evaluate '%s'\n", r->value); exit(1);
    return 0;
}

// ----------------- Free tree -----------------
void freeTree(Node *r){
    if(!r) return;
    if(r->left) freeTree(r->left);
    if(r->right) freeTree(r->right);
    free(r->value);
    free(r);
}

// ----------------- ASCII tree print -----------------
void printTree(Node *root,int space){
    if(!root) return;
    space+=8;
    printTree(root->right,space);
    printf("\n"); for(int i=8;i<space;i++) printf(" "); printf("%s\n",root->value);
    printTree(root->left,space);
}

// ----------------- MAIN -----------------
int main(){
    char expr[512];
    printf("Enter infix expression:\n");
    if(!fgets(expr,sizeof(expr),stdin)) return 0;
    size_t L=strlen(expr); if(L && expr[L-1]=='\n') expr[L-1]=0;

    Token tokens[MAXTOK];
    int tcount = tokenize(expr,tokens);
    printf("Tokens (%d): ",tcount);
    for(int i=0;i<tcount;i++) printf("'%s' ",tokens[i].text);
    printf("\n");

    char postfix[MAXTOK][MAXLEN];
    int pcount = infixToPostfix(tokens,tcount,postfix);

    printf("Postfix (%d): ",pcount);
    for(int i=0;i<pcount;i++) printf("%s ",postfix[i]);
    printf("\n");

    char* pfarr[MAXTOK];
    for(int i=0;i<pcount;i++) pfarr[i]=postfix[i];

    Node *root = buildExpressionTree(pfarr,pcount);

    printf("\nExpression Tree (ASCII):\n");
    printTree(root,0);

    printf("\nInfix before simplify: ");
    printInfix(root); printf("\n");

    root = simplify(root);

    printf("Infix after simplify: ");
    printInfix(root); printf("\n");

    int res = evaluate(root);
    printf("Evaluation result: %s\n", res?"true":"false");

    freeTree(root);
    return 0;
}
