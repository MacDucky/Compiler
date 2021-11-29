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
        next = nullptr;
    }

    Variable(string key, string type, int address, int size) {
        this->identifier = key;
        this->size = size;
        this->type = type;
        this->address = address;
        next = nullptr;
    }

    friend class SymbolTable;

};

class SymbolTable {
    /* Think! what can you add to  symbol_table */
    Variable *head[MAX];

public:
    SymbolTable() {
        for (int i = 0; i < MAX; i++)
            head[i] = nullptr;
    }

    // Function to find an identifier
    int find(string id) {
        int index = hashf(id);
        Variable *start = head[index];

        if (start == nullptr)
            return -1;

        while (start != nullptr) {

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

        if (head[index] == nullptr) {
            head[index] = p;
            return true;
        } else {
            Variable *start = head[index];
            while (start->next != nullptr)
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

    void print() const {
        cout << "print() placeholder" << endl;
    }
};

SymbolTable ST;

class TreeNode { //base class
public:
    /*you can add another son nodes */
    TreeNode *son1 = nullptr;
    TreeNode *son2 = nullptr;

    virtual ~TreeNode() {
        delete son1;
        delete son2;
    };

    TreeNode(TreeNode *left = nullptr, TreeNode *right = nullptr) : son1(left), son2(right) {};

    /*recursive function to make Pcode*/
    virtual void gencode(string c_type = "") {
        if (son1 != nullptr) son1->gencode(c_type);
        if (son2 != nullptr) son2->gencode(c_type);
    };

};

/*******************************************************    IMPLEMENTATION ZONE     ***************************************************************/
TreeNode *obj_tree(treenode *root);

class Print : public TreeNode {
public:
    explicit Print(treenode *rr_node) : TreeNode(nullptr, obj_tree(rr_node)) {}

    void gencode(string c_type) override {
        if (son2) son2->gencode("coder");
        cout << "print" << endl;
    }
};

class For : public TreeNode {
    TreeNode *increment, *statement;
    static int for_loop_idx;
    static int for_end_idx;
public:
    For(treenode *init, treenode *cond, treenode *increment, treenode *statement) : TreeNode(obj_tree(init),
                                                                                             obj_tree(cond)),
                                                                                    increment(obj_tree(increment)),
                                                                                    statement(obj_tree(statement)) {}

    void gencode(string c_type) override {
        const string &for_loop_label("for_loop" + to_string(for_loop_idx++));
        const string &for_end_label("for_end" + to_string(for_end_idx++));
        son1->gencode("");              //  init
        cout << for_loop_label + ":" << endl;
        son2->gencode("coder");         //  check condition
        cout << "fjp " + for_end_label << endl;
        statement->gencode("coder");    //  for body
        increment->gencode("coder");    //  increment
        cout << "ujp " + for_loop_label << endl;
        cout << for_end_label + ":" << endl;
    }

    ~For() override {
        delete increment;
        delete statement;
    }
};

int For::for_loop_idx = 0;
int For::for_end_idx = 0;

class While : public TreeNode {
    static int _while_loop_label_idx;
    static int _while_end_idx;
public:
    While(treenode *expression, treenode *statement) : TreeNode(obj_tree(expression), obj_tree(statement)) {}

    void gencode(string c_type) override {
        const string &while_loop_label("while_loop" + to_string(_while_loop_label_idx++));
        const string &while_end_label("while_end" + to_string(_while_end_idx++));
        cout << while_loop_label + ":" << endl;
        son1->gencode("coder");
        cout << "fjp " + while_end_label << endl;
        son2->gencode("coder");
        cout << "ujp " + while_loop_label << endl;
        cout << while_end_label + ":" << endl;
    }
};

int While::_while_loop_label_idx = 0;
int While::_while_end_idx = 0;

class DoWhile : public TreeNode {
    static int _dowhile_loop_label_idx;
public:
    DoWhile(treenode *expression, treenode *statement) : TreeNode(obj_tree(expression), obj_tree(statement)) {}

    void gencode(string c_type) override {
        const string &dowhile_label("do_while" + to_string(_dowhile_loop_label_idx++));
        cout << dowhile_label + ":" << endl;
        son2->gencode("coder");             //do stuff
        son1->gencode("coder");             //check whether to continue
        cout << "not" << endl;                     //if expression is true, then convert to false,and we can jump.
        cout << "fjp " + dowhile_label << endl;    //else just go to next code section.
    }
};

int DoWhile::_dowhile_loop_label_idx = 0;

class If : public TreeNode {
    TreeNode *_else_do;

    static int _if_end_label_idx;
    static int _ifelse_else_label_idx;
    static int _ifelse_end_label_idx;
public:
    If(treenode *cond, treenode *thenDo, treenode *elseDo = nullptr) : TreeNode(obj_tree(cond), obj_tree(thenDo)),
                                                                    _else_do(obj_tree(elseDo)) {}

    ~If() override {
        delete _else_do;
    }

    void gencode(string c_type) override {

        if (!_else_do) {// simple if case.
            if (son1 && son2) {
                son1->gencode("coder");
                const string &ifend_label("if_end" + to_string(_if_end_label_idx++));
                cout << "fjp " + ifend_label << endl;
                son2->gencode("coder");
                cout << ifend_label + ":" << endl;
            } else {
                throw "something wrong with AST node to TreeNode conversion.\n";
            }
        } else {// if else case.
            son1->gencode("coder");
            const string &ifelse_else_label("ifelse_else" + to_string(_ifelse_else_label_idx++));
            const string &ifelse_end_label("ifelse_end" + to_string(_ifelse_end_label_idx++));
            cout << "fjp " + ifelse_else_label << endl;
            son2->gencode("coder");
            cout << "ujp " + ifelse_end_label << endl;
            cout << ifelse_else_label + ":" << endl;
            _else_do->gencode("coder");
            cout << ifelse_end_label + ":" << endl;
        }
    }
};

int If::_if_end_label_idx = 0;
int If::_ifelse_else_label_idx = 0;
int If::_ifelse_end_label_idx = 0;


class Ternary : public TreeNode {
    TreeNode *_else_ret;

    static int _cond_else_idx;
    static int _condLabel_end_idx;
public:
    Ternary(treenode *cond, treenode *then_ret, treenode *otherwise_ret) : TreeNode(obj_tree(cond), obj_tree(then_ret)),
                                                                           _else_ret(obj_tree(otherwise_ret)) {}

    ~Ternary() override {
        delete _else_ret;
    }

    void gencode(string c_type) override {
        son1->gencode("coder");     // cond check
        const string &cond_else_label("cond_else" + to_string(_cond_else_idx++));
        const string &cond_end_label("condLabel_end" + to_string(_condLabel_end_idx++));
        cout << "fjp " + cond_else_label << endl;
        son2->gencode("coder");     // then return expr
        cout << "ujp " + cond_end_label << endl;
        cout << cond_else_label + ":" << endl;  // otherwise return expr
        _else_ret->gencode("coder");
        cout << cond_end_label + ":" << endl;
    }
};

int Ternary::_cond_else_idx = 0;
int Ternary::_condLabel_end_idx = 0;


/* Notice that this class expects rhs expressions. */
class BinOp : public TreeNode {
protected:
    string _op;
public:
    explicit BinOp(const string &pcode_op, treenode *left, treenode *right) : TreeNode(nullptr, nullptr),
                                                                              _op(pcode_op) {
        son1 = obj_tree(left);
        son2 = obj_tree(right);
    }

    void gencode(string c_type) override {
        if (!son1 && son2) { //only case is when its ' -x '
            son2->gencode("coder");
            cout << "neg" << endl;
        } else {
            if (son1 != nullptr) son1->gencode("coder");
            if (son2 != nullptr) son2->gencode("coder");
            cout << _op << endl;
        }
    }
};

class AssignBinOp : public BinOp {
public:
    explicit AssignBinOp(const string &op, treenode *lnode, treenode *rnode) : BinOp(op, lnode, rnode) {}

    void gencode(string c_type) override {
        if (son1 != nullptr) son1->gencode("codel");
        BinOp::gencode("");
        cout << "sto" << endl;
    }
};

/*
* you have to add functions/implement of gencode()... of derived classes
*/
class Assign : public TreeNode {
public:
    virtual void gencode(string c_type) {
        if (son1 != nullptr) son1->gencode("codel"); //return address
        if (son2 != nullptr) son2->gencode("coder"); // return value
        cout << "sto " << endl;
    }
};

class Id : public TreeNode {
    string id_name;
    static bool is_func_label;  // true when it's a func
public:

    Id(string id_n) {
        id_name = id_n;
        if (ST.find(id_n) == -1) {
            // todo: temporary fix for label, is it even needed?
            if (!is_func_label) {
                // you need to add the type and size according to declaration of identifier in AST
                ST.insert(id_name, "int", Stack_Address++, 1);
            } else
                is_func_label = false;
        }
    }

    virtual void gencode(string c_type) {
        if (c_type == "codel") {
            cout << "ldc " << ST.find(id_name) << endl;
        } else if (c_type == "coder") {
            cout << "ldc " << ST.find(id_name) << endl;
            cout << "ind" << endl;
        }
    }

    static void setIsFuncLabel() {
        is_func_label = true;
    }
};

bool Id::is_func_label = false;

class Num : public TreeNode {
    int value;
public:
    int getValue() const { return value; }

    explicit Num(int number) : TreeNode(), value(number) {}

    virtual void gencode(string c_type) {
        cout << "ldc " << getValue() << endl;
    }
};

class RealNum : public TreeNode {
    double value;
public:
    RealNum(double value) : TreeNode(), value(value) {}

    double getValue() const { return value; }

    virtual void gencode(string c_type) {
        cout << fixed;
        cout.precision(2);
        cout << "ldc " << getValue() << endl;
    }
};

/* This is a++ */
class Opxx : public TreeNode {
    const string op;
public:
    Opxx(const string &_op, treenode *lnode) : TreeNode(obj_tree(lnode), nullptr), op(_op) {}

    void gencode(string c_type) override {
        son1->gencode("coder");     //this is the ret_val
        son1->gencode("codel");     // get address
        son1->gencode("coder");     // get value
        cout << op + " 1" << endl;         // inc/dec by 1
        cout << "sto" << endl;             // store back
    }
};

/* This is ++a */
class xxOp : public TreeNode {
    const string op;
public:
    xxOp(const string &op, treenode *rnode) : TreeNode(nullptr, obj_tree(rnode)), op(op) {}

    void gencode(string c_type) override {
        son2->gencode("codel");
        son2->gencode("coder");
        cout << op + " 1" << endl;
        cout << "sto" << endl;
        son2->gencode("coder");
    }
};

class Not : public TreeNode {
public:
    virtual void gencode(string c_type) {
        if (!son1 && son2) son2->gencode("coder"); // return value
        else throw "something wrong with AST node to TreeNode construction!\n";
        cout << "not" << endl;
    }
};

/*****************************************************   END OF IMPLEMENTATION ZONE   ************************************************/


/********************************************************   RUN/CONSTRUCTION ZONE   **************************************************/


/*
*	Input: Tree of objects
*	Output: prints the Pcode on the console
*/
int code_recur(treenode *root) {
    TreeNode *tree_root = obj_tree(root);
    tree_root->gencode();
    delete tree_root;
    return SUCCESS;
}

/*
*	This recursive function is the main method for Code Generation
*	Input: treenode (AST)
*	Output: Tree of objects
*/
TreeNode *obj_tree(treenode *root) {
    if_node *ifn;
    for_node *forn;
    leafnode *leaf;
    if (!root) {
        return nullptr;
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
                    string s = leaf->data.sval->str;
                    return new Id(s);
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
                    TreeNode *const_number = new Num(leaf->data.ival);
                    return const_number;
                }

                case TN_REAL:
                    /* Constant case */
                    /*
                    *	In order to get the real value you have to use:
                    *	leaf->data.dval
                    */
                    TreeNode *real_number = new RealNum(leaf->data.dval);
                    return real_number;
            }
            break;

        case IF_T:
            ifn = (if_node *) root;
            switch (ifn->hdr.type) {

                case TN_IF:
                    if (ifn->else_n == nullptr) {
                        /* if case (without else)*/
                        return new If(ifn->cond, ifn->then_n);
                    } else {
                        /* if - else case*/
                        return new If(ifn->cond, ifn->then_n, ifn->else_n);
                    }

                case TN_COND_EXPR:
                    /* (cond)?(exp):(exp); */
                    return new Ternary(ifn->cond, ifn->then_n, ifn->else_n);

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

                case TN_FOR: {
                    /* For case*/
                    /* e.g. for(i=0;i<5;i++) { ... } */
                    /* Look at the output AST structure! */
                    return new For(forn->init, forn->test, forn->incr, forn->stemnt);
                }

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

                case TN_TRANS_LIST: {
                    /* Maybe you will use it later */
                    return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                }

                case TN_FUNC_DECL: {
                    /* Maybe you will use it later */
                    Id::setIsFuncLabel();   // temporary patch to **NOT** add func labels.
                    return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                }

                case TN_FUNC_CALL:
                    /* Function call */
                    if (strcmp(((leafnode *) root->lnode)->data.sval->str, "printf") == 0) {
                        /* printf case */
                        /* The expression that you need to print is located in */
                        /* the currentNode->right->right sub tree */
                        /* Look at the output AST structure! */
                        return new Print(root->rnode->rnode);
                    } else {
                        /* other function calls - for HW3 */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    }
                    break;

                case TN_BLOCK: {/* Maybe you will use it later */
                    TreeNode *t = new TreeNode();
                    t->son1 = obj_tree(root->lnode);
                    t->son2 = obj_tree(root->rnode);
                    return t;
                }

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

                case TN_TYPE_LIST: {
                    TreeNode *t = new TreeNode();
                    t->son1 = obj_tree(root->lnode);
                    t->son2 = obj_tree(root->rnode);
                    return t;
                }

                case TN_COMP_DECL:
                    /* struct component declaration - for HW2 */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;

                case TN_DECL: {   /* structs declaration - for HW2 */
                    // Dany: for future use.
                    string var_type;
                    if (root->lnode->hdr.type == TN_TYPE_LIST) { // var type
                        var_type = toksym(((leafnode *) root->lnode->lnode)->hdr.tok, 0);
                    }
                    return new TreeNode(nullptr, obj_tree(root->rnode));
                }

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

                case TN_STEMNT_LIST: {/* Maybe you will use it later */
                    return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                }

                case TN_STEMNT: {
                    /* Maybe you will use it later */
                    TreeNode *t = new TreeNode();
                    if (root->lnode) t->son1 = obj_tree(root->lnode);
                    if (root->rnode) t->son2 = obj_tree(root->rnode);
                    return t;
                }

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
                        TreeNode *ass_obj = new Assign();
                        ass_obj->son1 = obj_tree(root->lnode);
                        ass_obj->son2 = obj_tree(root->rnode);
                        return ass_obj;

                    } else if (root->hdr.tok == PLUS_EQ) {
                        /* Plus equal assignment "+=" */
                        /* e.g. x += 5; */
                        return new AssignBinOp("add", root->lnode, root->rnode);
                    } else if (root->hdr.tok == MINUS_EQ) {
                        /* Minus equal assignment "-=" */
                        /* e.g. x -= 5; */
                        return new AssignBinOp("sub", root->lnode, root->rnode);
                    } else if (root->hdr.tok == STAR_EQ) {
                        /* Multiply equal assignment "*=" */
                        /* e.g. x *= 5; */
                        return new AssignBinOp("mul", root->lnode, root->rnode);
                    } else if (root->hdr.tok == DIV_EQ) {
                        /* Divide equal assignment "/=" */
                        /* e.g. x /= 5; */
                        return new AssignBinOp("div", root->lnode, root->rnode);
                    }
                    break;

                case TN_EXPR:
                    switch (root->hdr.tok) {
                        case CASE:
                            /* you should not get here */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;

                        case INCR: {
                            /* Increment token "++" */
                            if (root->lnode && !root->rnode) { // Axx case
                                return new Opxx("inc", root->lnode);
                            } else {
                                if (!root->lnode && root->rnode) {
                                    return new xxOp("inc", root->rnode);
                                } else {
                                    throw "something went wrong in INCR";
                                }
                            }
                        }

                        case DECR: {
                            /* Decrement token "--" */
                            if (root->lnode && !root->rnode) { // Axx case
                                return new Opxx("dec", root->lnode);
                            } else {
                                if (!root->lnode && root->rnode) {
                                    return new xxOp("dec", root->rnode);
                                } else {
                                    throw "something went wrong in DECR";
                                }
                            }
                        }

                        case PLUS: {
                            /* Plus token "+" */
                            TreeNode *bin_obj = new BinOp("add", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case MINUS: {/* Minus token "-" */
                            TreeNode *bin_obj = new BinOp("sub", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case DIV: {/* Divide token "/" */
                            TreeNode *bin_obj = new BinOp("div", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case STAR: {/* multiply token "*" */
                            TreeNode *bin_obj = new BinOp("mul", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case AND: {
                            /* And token "&&" */
                            TreeNode *bin_obj = new BinOp("and", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case OR: {
                            /* Or token "||" */
                            TreeNode *bin_obj = new BinOp("or", root->lnode, root->rnode);
                            return bin_obj;
                        }


                        case NOT: {/* Not token "!" */
                            TreeNode *not_obj = new Not();
                            not_obj->son1 = obj_tree(root->lnode);
                            not_obj->son2 = obj_tree(root->rnode);
                            return not_obj;
                        }

                        case GRTR: {
                            /* Greater token ">" */
                            TreeNode *bin_obj = new BinOp("grt", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case LESS: {
                            /* Less token "<" */
                            TreeNode *bin_obj = new BinOp("les", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case EQUAL: {
                            /* Equal token "==" */
                            TreeNode *bin_obj = new BinOp("equ", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case NOT_EQ: {
                            /* Not equal token "!=" */
                            TreeNode *bin_obj = new BinOp("neq", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case LESS_EQ: {
                            /* Less or equal token "<=" */
                            TreeNode *bin_obj = new BinOp("leq", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        case GRTR_EQ: {
                            /* Greater or equal token ">=" */
                            TreeNode *bin_obj = new BinOp("geq", root->lnode, root->rnode);
                            return bin_obj;
                        }

                        default:
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;
                    }
                    break;

                case TN_WHILE:
                    /* While case */
                    return new While(root->lnode, root->rnode);

                case TN_DOWHILE: {
                    /* Do-While case */
//                    this is i+=1
//                    obj_tree(root->rnode);
//                    obj_tree(root->lnode);
//                    break;
                    return new DoWhile(root->lnode, root->rnode);
                }

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
void print_symbol_table(treenode *root) {
    printf("---------------------------------------\n");
    printf("Showing the Symbol Table:\n");

    ST.print();
}