#include <iostream>

using namespace std;

#include    "CodeGenerator.h"

static int Stack_Address = 5;
const int MAX = 100;


class Variable {

    /* Think! what does a Variable contain? */
    string identifier, type;
    int address, size;
    Variable *next;

public:
    Variable() {
        next = NULL;
    }

    Variable(string key, string type, int address, int size) {
        this->identifier = key;
        this->size = size;
        this->type = type;
        this->address = address;
        next = NULL;
    }

    friend class SymbolTable;

};

class SymbolTable {
    /* Think! what can you add to  symbol_table */
    Variable *head[MAX];

public:
    SymbolTable() {
        for (int i = 0; i < MAX; i++)
            head[i] = NULL;
    }

    // Function to find an identifier
    int find(string id) {
        int index = hashf(id);
        Variable *start = head[index];

        if (start == NULL)
            return -1;

        while (start != NULL) {

            if (start->identifier == id) {
                return start->address;
            }

            start = start->next;
        }

        return -1; // not found
    }

    // Function to insert an identifier
    bool insert(string id, string type, int address, int size) {
        int index = hashf(id);
        Variable *p = new Variable(id, type, address, size);

        if (head[index] == NULL) {
            head[index] = p;
            return true;
        } else {
            Variable *start = head[index];
            while (start->next != NULL)
                start = start->next;
            start->next = p;
            return true;
        }

        return false;
    }

    int hashf(string id) {
        int asciiSum = 0;

        for (int i = 0; i < id.length(); i++) {
            asciiSum = asciiSum + id[i];
        }

        return (asciiSum % MAX);
    }

};

SymbolTable ST;

/*******************************************************    IMPLEMENTATION ZONE     ***************************************************************/
class TreeNode { //base class
public:
    /*you can add another son nodes */
    TreeNode *son1 = NULL;
    TreeNode *son2 = NULL;

    virtual ~TreeNode() {};

    TreeNode() {};

    /*recursive function to make Pcode*/
    virtual void gencode(string c_type = "coder") {
        if (son1 != NULL) son1->gencode(c_type);
        if (son2 != NULL) son2->gencode(c_type);
    };

};

/*
* you have to add functions/implement of gencode()... of derived classes
*/
class Assign : public TreeNode {
public:
    virtual void gencode(string c_type) {
        if (son1 != NULL) son1->gencode("codel"); //return address
        if (son2 != NULL) son2->gencode("coder"); // return value
        cout << "sto " << endl;
    }
};

class Id : public TreeNode {
public:
    string id_name;

    Id(string id_n) {
        id_name = id_n;
        ST.insert(id_name, "int", Stack_Address++,
                  1); // you need to add the type and size according to declaration of identifier in AST
    }

    virtual void gencode(string c_type) {
        if (c_type == "codel") {
            cout << "ldc " << ST.find(id_name) << endl;
        }
    }
};

class Num : public TreeNode {
    int value;
public:
    int getValue() { return value; }

    Num(int number) { value = number; }

    virtual void gencode(string c_type) {
        cout << "ldc " << getValue() << endl;
    }
};

class RealNum : public TreeNode {
    double value;
public:
    RealNum(double value) : value(value) {}

    double getValue() const { return value; }

    virtual void gencode(string c_type) {
        cout << "ldc " << getValue() << endl;
    }
}


/*****************************************************   END OF IMPLEMENTATION ZONE   ************************************************/


/********************************************************   RUN/CONSTRUCTION ZONE   **************************************************/
TreeNode* obj_tree(treenode *root);

/*
*	Input: Tree of objects 
*	Output: prints the Pcode on the console
*/
int code_recur(treenode * root) {
    TreeNode * tree_root = obj_tree(root);
    tree_root->gencode();
    return SUCCESS;

}

/*
*	This recursive function is the main method for Code Generation
*	Input: treenode (AST)
*	Output: Tree of objects 
*/
TreeNode *obj_tree(treenode * root) {
    if_node *ifn;
    for_node *forn;
    leafnode *leaf;
    if (!root) {
        return NULL;
    }

    switch (root->hdr.which) {
        case LEAF_T:
            leaf = (leafnode *) root;
            switch (leaf->hdr.type) {
                case TN_LABEL:
                    /* Maybe you will use it later */
                    break;

                case TN_IDENT:
                    /* variable case */
                    /*
                    *	In order to get the identifier name you have to use:
                    *	leaf->data.sval->str
                    */
                {
                    TreeNode * ident = new Id(leaf->data.sval->str);
                    return ident;
                }


                case TN_COMMENT:
                    /* Maybe you will use it later */
                    break;

                case TN_ELLIPSIS:
                    /* Maybe you will use it later */
                    break;

                case TN_STRING:
                    /* Maybe you will use it later */
                    break;

                case TN_TYPE:
                    /* Maybe you will use it later */
                    break;

                case TN_INT:
                    /* Constant case */
                    /*
                    *	In order to get the int value you have to use:
                    *	leaf->data.ival
                    */
                {
                    TreeNode * const_number = new Num(leaf->data.ival);
                    return const_number;
                }

                case TN_REAL:
                    /* Constant case */
                    /*
                    *	In order to get the real value you have to use:
                    *	leaf->data.dval
                    */
                    TreeNode * real_number = new RealNum(leaf->data.dval);
                    return real_number;
            }
            break;

        case IF_T:
            ifn = (if_node *) root;
            switch (ifn->hdr.type) {

                case TN_IF:
                    if (ifn->else_n == NULL) {
                        /* if case (without else)*/
                        obj_tree(ifn->cond);
                        obj_tree(ifn->then_n);
                    } else {
                        /* if - else case*/
                        obj_tree(ifn->cond);
                        obj_tree(ifn->then_n);
                        obj_tree(ifn->else_n);
                    }
                    break;

                case TN_COND_EXPR:
                    /* (cond)?(exp):(exp); */
                    obj_tree(ifn->cond);
                    obj_tree(ifn->then_n);
                    obj_tree(ifn->else_n);
                    break;

                default:
                    /* Maybe you will use it later */
                    obj_tree(ifn->cond);
                    obj_tree(ifn->then_n);
                    obj_tree(ifn->else_n);
            }
            break;

        case FOR_T:
            forn = (for_node *) root;
            switch (forn->hdr.type) {

                case TN_FUNC_DEF:
                    /* Function definition */
                    /* e.g. int main(...) { ... } */
                    /* Look at the output AST structure! */
                    obj_tree(forn->init);
                    obj_tree(forn->test);
                    obj_tree(forn->incr);
                    obj_tree(forn->stemnt);
                    break;

                case TN_FOR:
                    /* For case*/
                    /* e.g. for(i=0;i<5;i++) { ... } */
                    /* Look at the output AST structure! */
                    obj_tree(forn->init);
                    obj_tree(forn->test);
                    obj_tree(forn->stemnt);
                    obj_tree(forn->incr);
                    break;

                default:
                    /* Maybe you will use it later */
                    obj_tree(forn->init);
                    obj_tree(forn->test);
                    obj_tree(forn->stemnt);
                    obj_tree(forn->incr);
            }
            break;

        case NODE_T:
            switch (root->hdr.type) {
                case TN_PARBLOCK:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_PARBLOCK_EMPTY:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_TRANS_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_FUNC_DECL:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_FUNC_CALL:
                    /* Function call */
                    if (strcmp(((leafnode *) root->lnode)->data.sval->str, "printf") == 0) {
                        /* printf case */
                        /* The expression that you need to print is located in */
                        /* the currentNode->right->right sub tree */
                        /* Look at the output AST structure! */
                        obj_tree(root->rnode->rnode);
                    } else {
                        /* other function calls - for HW3 */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    }
                    break;

                case TN_BLOCK:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_ARRAY_DECL:
                    /* array declaration - for HW2 */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_EXPR_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_NAME_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_ENUM_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_FIELD_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_PARAM_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_IDENT_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_TYPE_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_COMP_DECL:
                    /* struct component declaration - for HW2 */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_DECL:
                    /* structs declaration - for HW2 */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_DECL_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_DECLS:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_STEMNT_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_STEMNT:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_BIT_FIELD:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_PNTR:
                    /* pointer - for HW2! */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_TYPE_NME:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_INIT_LIST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_INIT_BLK:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_OBJ_DEF:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_OBJ_REF:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_CAST:
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_JUMP:
                    if (root->hdr.tok == RETURN) {
                        /* return jump - for HW2! */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else if (root->hdr.tok == BREAK) {
                        /* break jump - for HW2! */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else if (root->hdr.tok == GOTO) {
                        /* GOTO jump - for HW2! */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    }
                    break;

                case TN_SWITCH:
                    /* Switch case - for HW2! */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_INDEX:
                    /* call for array - for HW2! */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_DEREF:
                    /* pointer derefrence - for HW2! */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_SELECT:
                    /* struct case - for HW2! */
                    if (root->hdr.tok == ARROW) {
                        /* Struct select case "->" */
                        /* e.g. struct_variable->x; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else {
                        /* Struct select case "." */
                        /* e.g. struct_variable.x; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    }
                    break;

                case TN_ASSIGN:
                    if (root->hdr.tok == EQ) {
                        /* Regular assignment "=" */
                        /* e.g. x = 5; */
                        TreeNode * ass_obj = new Assign();
                        ass_obj->son1 = obj_tree(root->lnode);
                        ass_obj->son2 = obj_tree(root->rnode);
                        return ass_obj;

                    } else if (root->hdr.tok == PLUS_EQ) {
                        /* Plus equal assignment "+=" */
                        /* e.g. x += 5; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else if (root->hdr.tok == MINUS_EQ) {
                        /* Minus equal assignment "-=" */
                        /* e.g. x -= 5; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else if (root->hdr.tok == STAR_EQ) {
                        /* Multiply equal assignment "*=" */
                        /* e.g. x *= 5; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else if (root->hdr.tok == DIV_EQ) {
                        /* Divide equal assignment "/=" */
                        /* e.g. x /= 5; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    }
                    break;

                case TN_EXPR:
                    switch (root->hdr.tok) {
                        case CASE:
                            /* you should not get here */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case INCR:
                            /* Increment token "++" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case DECR:
                            /* Decrement token "--" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case PLUS:
                            /* Plus token "+" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case MINUS:
                            /* Minus token "-" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case DIV:
                            /* Divide token "/" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case STAR:
                            /* multiply token "*" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case AND:
                            /* And token "&&" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case OR:
                            /* Or token "||" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case NOT:
                            /* Not token "!" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case GRTR:
                            /* Greater token ">" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case LESS:
                            /* Less token "<" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case EQUAL:
                            /* Equal token "==" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case NOT_EQ:
                            /* Not equal token "!=" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case LESS_EQ:
                            /* Less or equal token "<=" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case GRTR_EQ:
                            /* Greater or equal token ">=" */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        default:
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;
                    }
                    break;

                case TN_WHILE:
                    /* While case */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_DOWHILE:
                    /* Do-While case */
                    obj_tree(root->rnode);
                    obj_tree(root->lnode);
                    break;

                case TN_LABEL:
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                default:
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
            }
            break;

        case NONE_T:
            printf("Error: Unknown node type!\n");
            exit(FAILURE);
    }


}


/*
*	This function prints all the variables on your symbol table with their data
*	Input: treenode (AST)
*	Output: prints the Symbol Table on the console
*/
void print_symbol_table(treenode * root) {
    printf("---------------------------------------\n");
    printf("Showing the Symbol Table:\n");

    /*
    *	add your code here
    */
}
