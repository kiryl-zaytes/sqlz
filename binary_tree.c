# include <stdlib.h>
# include <string.h>
# include <stdio.h>
# include <sys/stat.h>

struct tnode{
    int count;
    char *word;
    struct tnode *left;
    struct tnode *right;
};

struct tnode *talloc(void){
    return (struct tnode *) malloc(sizeof(struct tnode));
};

char *strdupp(char *s){
    char *c;
    c = (char *) malloc(strlen(s) + 1);
    if (c != NULL) strcpy(c, s);
    return c;
}


struct tnode *addtree(struct tnode *node, char *value){
    int cond;
    if (node == NULL){
        node = talloc();
        node->word=strdupp(value);
        node->count=1;
        node->left=NULL;
        node->right=NULL;
    }
    else if ((cond=strcmp(value, node->word)) == 0)node->count++;
    else if (cond < 0) node->left = addtree(node->left, value);
    else node->right = addtree(node->right, value);
    return node;
}

void main(int argc, char *argv[]){
    char *string = "acbcd";
    char *p;
    p =strdupp(string);
    printf("%c", p);
}