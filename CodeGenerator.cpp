#include <iostream>
#include <vector>
#include <stack>
#include <list>
#include <regex>
#include <algorithm>

using namespace std;

#include    "CodeGenerator.h"

static int Stack_Address = 5;
static int Struct_Stack_Address = 0;
const int MAX = 100;

/*****************************************************   Aux Classes   ************************************************/
vector<string> tokenize(const string str, const regex re) {
    sregex_token_iterator it{str.begin(), str.end(), re, -1};
    vector<string> tokenized{it, {}};

    // Additional check to remove empty strings
    tokenized.erase(std::remove_if(tokenized.begin(), tokenized.end(),
                                   [](std::string const &s) {
                                       return s.size() == 0;
                                   }), tokenized.end());
    return tokenized;
}

template<class T>
class MyStack : public stack<T> {
public:

    // Old pop was weird, so I changed it.
    T pop() {
        T elem = this->top();
        stack<T>::pop();
        return elem;
    }

    void collect(const T &to_collect) {
        this->push(to_collect);
    }
};

typedef MyStack<int> Dims;
typedef MyStack<treenode *> ASTNodes;

template<class T>
class OrderedArray : public vector<T> {
public:
    OrderedArray() = default;

    explicit OrderedArray(const MyStack<T> &st) {
        MyStack<T> copy(st);
        while (!copy.empty()) {
            vector<T>::push_back(copy.pop());
        }
    }
};

class OrderedDims : public OrderedArray<int> {
public:
    OrderedDims() = default;

    OrderedDims(const OrderedDims &copy) = default;

    explicit OrderedDims(const Dims &st) : OrderedArray<int>(st) {}
};

class OrderedNodes : public OrderedArray<treenode *> {
public:
    OrderedNodes() = default;

    explicit OrderedNodes(const ASTNodes &astNodes) : OrderedArray<treenode *>(astNodes) {}
};

/**********************************************************************************************************************/


class Variable {

    /* Think! what does a Variable contain? */
    const string identifier, type;
    int address, size;
    Variable *next;
    OrderedDims ordered_dims;
    bool heap_allocd;
    list<Variable> fields;

public:
    Variable(string key, string type, int address, int size, const Dims &dims = Dims(), bool heap_allocd = false,
             list<Variable> fields = list<Variable>())
            : identifier(key), size(size),
              type(type), address(address),
              next(nullptr),
              ordered_dims(dims), heap_allocd(heap_allocd), fields(fields) {}

    Variable(string key, string type, int address, int size, const OrderedDims &o_dims, bool heap_allocd = false,
             list<Variable> fields = list<Variable>())
            : identifier(key), size(size),
              type(type), address(address),
              next(nullptr),
              ordered_dims(o_dims), heap_allocd(heap_allocd), fields(fields) {}

    const string &getIdentifier() const { return identifier; }

    const string &getType() const { return type; }

    int getAddress() const { return address; }

    int getSize() const { return size; }

    const OrderedDims &getOrderedDims() const { return ordered_dims; }

    bool is_heap_allocd() const { return heap_allocd; }

    static list<Variable> get_fields(int absolute_address, const list<Variable> &fields) {
        int abs_address = absolute_address;  // struct starting address.
        list<Variable> abs_fields;
        for (const auto &field: fields) {
            Variable copy(field);
            copy.address = abs_address;
            abs_address += field.size;
            abs_fields.push_back(copy);
        }
        return abs_fields;
    }

    const list<Variable> &get_fields_rel() const {
        return fields;
    }

    friend class SymbolTable;
};

class StructDef {
    string name;
    int curr_rel_address;
    int size;
    list<Variable> fields;
    static bool in_progress;
public:
    explicit StructDef(const string &_name) : name("struct " + _name), curr_rel_address(0), size(-1) {}

    StructDef(const string &_name, const list<Variable> &var_list) : name(_name), curr_rel_address(0), size(-1) {
        for (const auto &var: var_list) {
            add_field(var);
        }
    }

    void add_field(const Variable &field, bool single_field = false) {
        const string &identifier(field.getIdentifier()), &type(field.getType());
        int f_size(field.getSize());
        fields.emplace_back(identifier, type, curr_rel_address, f_size, field.getOrderedDims());
        curr_rel_address += field.getSize();
        if (single_field)
            size = curr_rel_address;
    }

    const string &get_name() const {
        return name;
    }

    Variable get_field(const string &field_name) const {
        for (const auto &field: fields) {
            if (field_name == field.getIdentifier()) {
                return field;
            }
        }
        cout << "Field unfound!" << endl;
        exit(-1);
    }

    int get_size() const {
        if (size == -1) {
            cout << "Struct not constructed yet!" << endl;
            exit(-1);
        }
        return size;
    }

    list<Variable> get_fields() const { return fields; }

    static bool is_in_progress() {
        return in_progress;
    }

    void start() {
        in_progress = true;
    }

    void finish() {
        in_progress = false;
        size = curr_rel_address;    // at the end should be total size.
        Struct_Stack_Address = 0;
    }

    ~StructDef() {}
};

bool StructDef::in_progress = false;

class StructFieldsGenerated {
    list<Variable> fields;
public:
    void add_field(const Variable &field) { fields.push_back(field); }

    void reset() { fields.clear(); }

    list<Variable> get_sfg() const { return fields; }
};

class StructDefinitions {
    list<StructDef> struct_defs;
public:
    void add_definition(const StructDef &struct_def) { struct_defs.push_back(struct_def); }

    const StructDef &find_def(const string &strct_type) const {
        for (const auto &def: struct_defs) {
            if (strct_type.find(def.get_name()) != string::npos)
                return def;
        }
        cout << "Struct definition not found!" << endl;
        exit(-1);
    }
};

class SymbolTable {
    /* Think! what can you add to  symbol_table */
    Variable *head[MAX];
//    list<StructDef> struct_definitions;
    StructFieldsGenerated sfg;
    StructDefinitions sd;

    struct {
        const string i;
        const string f;
        const string d;
        const string p;

        bool isBasic(const string &type) { return type == i || type == f || type == d || type == p; }
    } base_t;

    friend class Array;

public:
    SymbolTable() : base_t({"int", "float", "double", "pointer"}) {
        for (int i = 0; i < MAX; i++) {
            head[i] = nullptr;
        }
    }

    // Function to find a variable, nullptr is returned if not found.
    const Variable *find(const string &id) {
        bool is_struct_field = false;
        string struct_id, struct_field;
        auto iter = id.find('.');
        if (iter != string::npos) {
            struct_id = id.substr(0, iter);
            struct_field = id.substr(iter + 1);
            is_struct_field = true;
        }
        int index = is_struct_field ? hashf(struct_id) : hashf(id);
        Variable *start = head[index];

        if (start == nullptr)
            return nullptr;

        while (start != nullptr) {
            if (start->identifier == struct_id && is_struct_field) {
                string struct_type = start->getType();
                const StructDef &s_def = get_struct_definition(struct_type);
                const Variable field = s_def.get_field(struct_field);

                string f_id = field.identifier;
                string f_type = field.type;
                int f_address = start->address + field.address;
                int f_size = field.size;
                OrderedDims f_dims = field.getOrderedDims();

                return new Variable(f_id, f_type, f_address, f_size, f_dims, true);
            }
            if (start->identifier == id) {
                return start;
            }
            start = start->next;
        }
        return nullptr; // not found
    }

    // Function to insert an identifier
    bool insert(const string &id, const string &type, int address, int size, const Dims &dims = Dims(),
                list<Variable> fields = list<Variable>()) {
        int index = hashf(id);
        list<Variable> fields_to_insert = StructDef::is_in_progress() ? fields : Variable::get_fields(address, fields);
        Variable *p = new Variable(id, type, address, size, dims, false, fields_to_insert);
        if (StructDef::is_in_progress()) {
            sfg.add_field(*p);
            delete p;
            return true;
        }

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

    // Function to insert an identifier
    bool insert(Variable *var) {
        int index = hashf(var->identifier);
        Variable *p = var;
        Variable **ref;
        if (StructDef::is_in_progress()) {
            sfg.add_field(*p);
            delete p;
            return true;
        } else {
            ref = head;
        }

        if (ref[index] == nullptr) {
            ref[index] = p;
            return true;
        } else {
            Variable *start = ref[index];
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

    static void print() {
        cout << "print() placeholder" << endl;
    }

    static string CalcType(string type, int num_of_stars) {
        for (int i = 0; i < num_of_stars; ++i) {
            type += "*";
        }
        return type;
    }

    int CalcSize(const string &type, const Dims &dims_of_array = Dims()) {
        string t(type);
        int type_size = 0;
        Dims dims(dims_of_array);

        // might make problems currently with int* arr[5]!
        if (t.find('*') != string::npos) t = "pointer";
        if (base_t.isBasic(t))  // int,float,double,pointer
            type_size = 1;
        else {
            type_size = sd.find_def(type).get_size();
        }

        //maybe insert dims into table,can't hurt right? meh...
        while (!dims.empty()) {
            type_size *= dims.pop();
        }

        if (type_size == 0) {
            cout << "Size was not defined!" << endl;
            exit(-1);
        }
        return type_size;
    }

    bool is_pointer(const string &id) {
        const Variable *var = find(id);
        if (!var) {
            cout << "Var not in symbol table!" << endl;
            exit(-1);
        }
        const string &var_type = var->getType();

        if (var->is_heap_allocd())
            delete var;

        if (var_type.find('*') != string::npos)
            return true;
        return false;
    }

    bool is_array(const string &id) {
        const Variable *var = find(id);
        if (!var) {
            cout << "Var not in symbol table!" << endl;
            exit(-1);
        }
        OrderedDims var_dims = var->getOrderedDims();

        if (var->is_heap_allocd())
            delete var;

        if (!var_dims.empty())
            return true;
        return false;
    }

    list<Variable> get_generated_fields() const { return sfg.get_sfg(); }

    ~SymbolTable() {
        for (auto var: head) {
            if (var) delete var->next;
            delete var;
        }
    }

    void add_struct_definition(const StructDef &structDef) {
        sd.add_definition(structDef);
        sfg.reset();
    }

    const StructDef &get_struct_definition(string struct_type) const {
        return sd.find_def(struct_type);
    }
};

SymbolTable ST;

class TreeNode { //base class
public:
    /*you can add another son nodes */
    TreeNode *son1;
    TreeNode *son2;

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

/*
 * Generic class to have the option to collect according to tree structure.
 * Inherit from this class to add functionality.
*/
template<class CollectorObject, class LeftField = string, class CollectedType = int>
class Collector {
    const treenode *start;
    CollectorObject &collector_o;     // container with collect method
    LeftField &left_field;
protected:
    tn_t follow_up_node;
public:
    Collector(const treenode *start, CollectorObject &collector, tn_t follow_up_node_type, LeftField &left_field)
            : start(start),
              collector_o(collector),
              follow_up_node(
                      follow_up_node_type),
              left_field(left_field) {}

    void collect() {
        const treenode *curr = start;
        CollectedType item_to_collect;
        while (curr->lnode->hdr.type == follow_up_node) { // collect
            item_to_collect = right_field_selector(curr->rnode);
            collector_o.collect(item_to_collect);
            curr = curr->lnode;
        }
        collector_o.collect(right_field_selector(curr->rnode));
        left_field = left_field_selector(curr->lnode);
    }

    virtual CollectedType right_field_selector(treenode *to_collect_from) = 0;

    virtual LeftField left_field_selector(treenode *to_collect_from) = 0;
};

typedef list<treenode *> FieldNodes;

class StructComponentCollectorO {
    FieldNodes components;
public:
    void collect(treenode *component) { components.push_front(component); }

    const list<treenode *> &get_components() const { return components; }
};

class StructFieldCollector : public Collector<StructComponentCollectorO, treenode *, treenode *> {
    StructComponentCollectorO collector;
    treenode *left_field;
public:
    StructFieldCollector(treenode *field_list_node) : Collector<StructComponentCollectorO, treenode *, treenode *>(
            field_list_node, collector, TN_FIELD_LIST, left_field) {
        collect();
        collector.collect(left_field);
    }

    treenode *right_field_selector(treenode *to_collect_from) override {
        if (to_collect_from->hdr.type != TN_COMP_DECL) {
            cout << "RNode Selector: Wrong node collected!" << endl;
            exit(-1);
        }
        return to_collect_from;
    }

    treenode *left_field_selector(treenode *to_collect_from) override {
        if (to_collect_from->hdr.type != TN_COMP_DECL) {
            cout << "LNode Selector: Wrong node collected!" << endl;
            exit(-1);
        }
        return to_collect_from;
    }

    const FieldNodes &get_field_nodes() const {
        return collector.get_components();
    }

};

// This class adds labels to ST, it does NOT create pcode.
class ArrayLabelAdder : public Collector<Dims> {
    Dims dims;
    string type;
    string name;

public:
    ArrayLabelAdder(treenode *arr_decl_node, const string &type) : Collector<Dims>(arr_decl_node, dims,
                                                                                   TN_ARRAY_DECL, name), type(type) {
        collect();  // collect dimensions + name.
        int size = ST.CalcSize(type, dims);
        int &address = StructDef::is_in_progress() ? Struct_Stack_Address : Stack_Address;
        ST.insert(name, type, address, size, dims);
        address += size;
    }

    int right_field_selector(treenode *to_collect_from) override {
        return reinterpret_cast<leafnode *>(to_collect_from)->data.ival;
    }

    string left_field_selector(treenode *to_collect_from) override {
        return reinterpret_cast<leafnode *>(to_collect_from)->data.sval->str;
    }
};

class ArrayIndexCollector : public Collector<ASTNodes, string, treenode *> {
    ASTNodes treenodes;
    string name;
public:
    explicit ArrayIndexCollector(treenode *index_node) : Collector<ASTNodes, string, treenode *>(index_node, treenodes,
                                                                                                 TN_INDEX,
                                                                                                 name) {
        collect();
    }

    treenode *right_field_selector(treenode *to_collect_from) override {
        return to_collect_from;
    }

    string left_field_selector(treenode *to_collect_from) override {
        if (!to_collect_from)
            return "";
        if (to_collect_from->hdr.type == TN_SELECT) {
            string field = reinterpret_cast<leafnode *>(to_collect_from->rnode)->data.sval->str;
            bool is_dot = to_collect_from->hdr.tok == DOT;
            string op = is_dot ? "." : "->";
            if (to_collect_from->lnode && to_collect_from->lnode->hdr.type == TN_SELECT) {
                return left_field_selector(to_collect_from->lnode) + op + field;
            }
            string instance_name = reinterpret_cast<leafnode *>(to_collect_from->lnode)->data.sval->str;
            return instance_name + op + field;
        } else {
            return reinterpret_cast<leafnode *>(to_collect_from)->data.sval->str;
        }
    }

    OrderedNodes get_ordered_nodes() const { return OrderedNodes(treenodes); }

    string get_var_name() const { return name; }
};


class LoopBreak : public TreeNode {
    static MyStack<string> label_refs;
public:
    void gencode(string c_type) override {
        const string &last_label = label_refs.top();
        cout << "ujp " << last_label << std::endl;
    }

    static void AddLastLabel(const string &end_label) {
        label_refs.push(end_label);
    }

    static void RemoveLastLabel() { label_refs.pop(); }
};

MyStack<string> LoopBreak::label_refs;

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
                                                                                    statement(
                                                                                            obj_tree(
                                                                                                    statement)) {}

    void gencode(string c_type) override {
        const string &for_loop_label("for_loop" + to_string(for_loop_idx++));
        const string &for_end_label("for_end" + to_string(for_end_idx++));

        LoopBreak::AddLastLabel(for_end_label);

        son1->gencode("");              //  init
        cout << for_loop_label + ":" << endl;
        son2->gencode("coder");         //  check condition
        cout << "fjp " + for_end_label << endl;
        statement->gencode("coder");    //  for body

        LoopBreak::RemoveLastLabel();             //if a break was in the loop, it would occur by now.

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
    While(treenode *expression, treenode *statement) : TreeNode(obj_tree(expression),
                                                                obj_tree(statement)) {}

    void gencode(string c_type) override {
        const string &while_loop_label("while_loop" + to_string(_while_loop_label_idx++));
        const string &while_end_label("while_end" + to_string(_while_end_idx++));

        LoopBreak::AddLastLabel(while_end_label);

        cout << while_loop_label + ":" << endl;
        son1->gencode("coder");
        cout << "fjp " + while_end_label << endl;
        son2->gencode("coder");

        LoopBreak::RemoveLastLabel();             //if a break was in the loop, it would occur by now.

        cout << "ujp " + while_loop_label << endl;
        cout << while_end_label + ":" << endl;
    }
};

int While::_while_loop_label_idx = 0;
int While::_while_end_idx = 0;

class DoWhile : public TreeNode {
    static int _dowhile_loop_label_idx;
public:
    DoWhile(treenode *expression, treenode *statement) : TreeNode(obj_tree(expression),
                                                                  obj_tree(statement)) {}

    void gencode(string c_type) override {
        const string &dowhile_label("do_while" + to_string(_dowhile_loop_label_idx));
        const string dowhile_endlabel("do_while_end" + to_string(_dowhile_loop_label_idx++));

        LoopBreak::AddLastLabel(dowhile_endlabel);

        cout << dowhile_label + ":" << endl;
        son2->gencode("coder");             //do stuff
        son1->gencode("coder");             //check whether to continue

        LoopBreak::RemoveLastLabel();             //if a break was in the loop, it would occur by now.

        cout << "not" << endl;                     //if expression is true, then convert to false,and we can jump.
        cout << "fjp " + dowhile_label << endl;    //else just go to next code section.
        cout << dowhile_endlabel + ":" << endl;
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
    Ternary(treenode *cond, treenode *then_ret, treenode *otherwise_ret) : TreeNode(obj_tree(cond),
                                                                                    obj_tree(then_ret)),
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

class Index : public TreeNode {
    static int size_of_basic;
    static OrderedDims *current_dims;
    int _dim_ptr;
public:
    Index(treenode *lnode, treenode *rnode) : TreeNode(obj_tree(lnode), obj_tree(rnode)) {
        if (!current_dims) {
            _dim_ptr = -1;
        } else {
            _dim_ptr = 1;
        }
    }

    void gencode(string c_type) override {
        son1->gencode("codel");
        son2->gencode("coder");
        if (size_of_basic == 0) {
            cout << "Size was not defined!" << endl;
            exit(-1);
        }
        if (_dim_ptr == -1) {
            cout << "ixa " << size_of_basic << endl;
        } else {
            cout << "ixa " << (*current_dims)[_dim_ptr] * size_of_basic << endl;
            if ((*current_dims).size() - 1 == _dim_ptr) {
                _dim_ptr = -1;
            } else {
                _dim_ptr++;
            }
        }
        if (c_type == "coder")
            cout << "ind" << endl;
    }

    static void setSizeOfBasic(int sizeOfBasic) {
        size_of_basic = sizeOfBasic;
    }

    static void setCurrentDims(OrderedDims *currentDims) {
        current_dims = currentDims;
    }

};

int Index::size_of_basic = 0;
OrderedDims *Index::current_dims = nullptr;


class Id : public TreeNode {
    string id_name;
    OrderedNodes orderedNodes;

    const Variable *_var;

public:
    static int num_of_derefs;

    explicit Id(const string id_n) : id_name(id_n) {}

    Id(const string id_n, const OrderedNodes &ordered_nodes) : id_name(id_n),
                                                               orderedNodes(ordered_nodes),
                                                               _var(nullptr) {}

    virtual void gencode(string c_type) {
        const Variable *var = ST.find(id_name);
        if (var == nullptr) {
            cout << "Variable was not declared!" << endl;
            exit(-1);
        }
        if (c_type == "codel") {
            cout << "ldc " << var->getAddress() << endl;
        } else if (c_type == "coder") {
            cout << "ldc " << var->getAddress() << endl;
            cout << "ind" << endl;
        }
        if (var->is_heap_allocd())
            delete var;
    }

};


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

static int switch_num = 0;
static int case_num = 0;

class SwitchCond : public TreeNode {

public:
    virtual void gencode(string c_type) {
        if (son1 != NULL) son1->gencode("coder");
        if (son2 != NULL) son2->gencode("coder");
        cout << "switch_end" + to_string(switch_num) + ":" << endl;
        LoopBreak::RemoveLastLabel();
        switch_num++;
        case_num = 0;
    }

};

class SwitchLabel : public TreeNode {

public:
    virtual void gencode(string c_type) {
        int cur_switch = switch_num;
        int cur_case = case_num;
        cout << "switch" + to_string(cur_switch) + "_case" + to_string(cur_case) + ":" << endl;
        cout << "dpl" << endl;
        if (son1 != NULL) son1->gencode("coder");
        cout << "equ" << endl;
        cout << "fjp switch" + to_string(cur_switch) + "_case" + to_string(cur_case + 1) << endl;
        LoopBreak::AddLastLabel("switch_end" + to_string(cur_switch));
        case_num++;
        if (son2 != NULL) son2->gencode("coder");
    }

};


class StructDot : public TreeNode {
    string left_field;
    string right_field;

    string get_field() const { return dynamic_cast<StructDot *>(son1)->right_field; }

public:

    StructDot(const string &l_field, const string &r_field, treenode *lnode = nullptr) : TreeNode(obj_tree(lnode),
                                                                                                  nullptr),
                                                                                         left_field(""),
                                                                                         right_field(r_field) {
        if (!l_field.empty()) {
            this->left_field = l_field;
        }
    }

    void gencode(string c_type) override {
        if (son1 != nullptr) son1->gencode(c_type);

        Variable *strct = const_cast<Variable *>(ST.find(left_field));    // instance

//        if (son1){
//            const StructDef& field_def = ST.get_struct_definition(get_field());
//            list<Variable> field of fields
//        }

        int abs_address = strct->getAddress();
        string struct_type = strct->getType();  // struct S

        const StructDef &s_def = ST.get_struct_definition(struct_type);
        list<Variable> fields = s_def.get_fields();

        if (!son1)
            cout << "ldc " << abs_address << endl;  // address of instance

        int rel_address = 0;
        for (const auto &_field: fields) {
            if (right_field == _field.getIdentifier())
                break;
            rel_address += _field.getSize();
        }

        cout << "inc " << rel_address << endl;
        if (c_type == "coder") {
            cout << "ind" << endl;
        }
    }
};

class PtrDeref : public TreeNode {
public:
    explicit PtrDeref(treenode *rnode) : TreeNode(nullptr, obj_tree(rnode)) {}

    void gencode(string c_type) override {
        son2->gencode(c_type);
        cout << "ind" << endl;
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

int build_tree_recur(treenode *root) {
    TreeNode *tree_root = obj_tree(root);
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


                case TN_COMMENT: {
                    /* This prints out comments in the c code, can be used to differentiate pieces of pcode output. */
                    string comment = leaf->data.str;
                    cout << comment << endl;
                    return nullptr;
                }

                case TN_ELLIPSIS:
                    /* Maybe you will use it later */
                    break;

                case TN_STRING: {
                    /* Maybe you will use it later */
                    break;
                }

                case TN_TYPE: {
                    /* This is function return type / variable declaration type */
                    string type = toksym(leaf->hdr.tok, 0);
                    return nullptr;
                    break;
                }

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
                case TN_PARBLOCK: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_PARBLOCK_EMPTY: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_TRANS_LIST: {
                    /* Maybe you will use it later */
                    return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                }

                case TN_FUNC_DECL: {
                    /* Maybe you will use it later */
                    return nullptr; // this is where you get info about functions...not relevant currently.
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

                case TN_ARRAY_DECL: {
                    /* array declaration - for HW2 */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_EXPR_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_NAME_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_ENUM_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_FIELD_LIST: {
                    /* Maybe you will use it later */
                    return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                }

                case TN_PARAM_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_IDENT_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_TYPE_LIST: {
                    TreeNode *treeNode = new TreeNode();
                    treeNode->son1 = obj_tree(root->lnode);
                    treeNode->son2 = obj_tree(root->rnode);
                    return treeNode;
                }

                case TN_COMP_DECL:  // same as regular decl
                case TN_DECL: {   /* structs declaration - for HW2 */
                    string var_type, var_name;
                    int num_of_stars = 0;

                    // var type
                    if (root->lnode->hdr.type == TN_TYPE_LIST) {
                        var_type = toksym(((leafnode *) root->lnode->lnode)->hdr.tok, 0);
                    }

                    // if its a struct decl/instantiation and NOT a pointer to a struct.
                    if (var_type == "struct" && root->lnode->lnode->hdr.type == TN_OBJ_REF) {
                        // Instantiation.
                        string struct_type = reinterpret_cast<leafnode *>(root->lnode->lnode->lnode)->data.sval->str;
                        var_type += " " + struct_type;
                    } else if (var_type == "struct" && root->lnode->lnode->hdr.type == TN_OBJ_DEF) { // Definition.
                        return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                    }


                    tn_t rn_type = root->rnode->hdr.type;
                    if (rn_type == TN_DECL) { // this is a pointer declaration.

                        auto count_stars = [](treenode *root, auto recur_count_stars) { // e.g. int ****b will yield 4.
                            if (!root->rnode)
                                return 1;
                            return recur_count_stars(root->rnode, recur_count_stars) + 1;
                        };

                        var_name = ((leafnode *) root->rnode->rnode)->data.sval->str;
                        num_of_stars = count_stars(root->rnode->lnode, count_stars);
                    } else if (rn_type == TN_IDENT) { // the node to the right contains the name
                        var_name = ((leafnode *) root->rnode)->data.sval->str;
                    } else if (rn_type == TN_ARRAY_DECL) { // this is an array declaration
                        ArrayLabelAdder(root->rnode, var_type);
                        return nullptr;
                    }

                    // Now we got all var info...
                    if (!var_name.empty() and !var_type.empty()) {
                        // adding to symbol table.
                        string final_type = ST.CalcType(var_type, num_of_stars);
                        int var_size = ST.CalcSize(final_type);
                        if (StructDef::is_in_progress()) {
                            ST.insert(var_name, final_type, Struct_Stack_Address, var_size);
                            Struct_Stack_Address += var_size;
                        } else {
                            const StructDef &s_def = ST.get_struct_definition(var_type);
                            list<Variable> fields = s_def.get_fields();
                            ST.insert(var_name, final_type, Stack_Address, var_size, Dims(), fields);
                            Stack_Address += var_size;
                        }

                    }

                    return new TreeNode(obj_tree(root->lnode), obj_tree(root->rnode));
                }

                case TN_DECL_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_DECLS: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

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

                case TN_BIT_FIELD: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_PNTR: {
                    /* pointer - for HW2! */
                    /* dany: nothing to be done here*/
                    return nullptr;
                }

                case TN_TYPE_NME: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_INIT_LIST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_INIT_BLK: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_OBJ_DEF: {
                    /* Maybe you will use it later */
                    if (string(toksym(root->hdr.tok, 0)) == "struct") {
                        // Give the struct a name.
                        string struct_name(reinterpret_cast<leafnode *>(root->lnode)->data.sval->str);
                        StructDef struct_definer(struct_name);
                        struct_definer.start();

                        if (root->rnode->hdr.type == TN_COMP_DECL) { // Single field declaration, skip collector.
                            build_tree_recur(root->rnode);
                            struct_definer.add_field(ST.get_generated_fields().front(), true);
                            ST.add_struct_definition(struct_definer);
                            struct_definer.finish();
                            return nullptr;
                        }

                        // Collect (field) component nodes.
                        StructFieldCollector collector(root->rnode);
                        const FieldNodes &field_nodes = collector.get_field_nodes();

                        for (const auto &node: field_nodes) {
                            build_tree_recur(node);
                        }

                        list<Variable> fields = ST.get_generated_fields();
                        for (auto &field: fields) {
                            struct_definer.add_field(field);
                        }

                        // symbol table already clears sfg...
                        struct_definer.finish();
                        ST.add_struct_definition(struct_definer);

                        return nullptr;
                    } else  // enums or something else don't matter...
                        return nullptr;
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_OBJ_REF: {
                    /* Maybe you will use it later */

                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_CAST: {
                    /* Maybe you will use it later */
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                    break;
                }

                case TN_JUMP: {
                    if (root->hdr.tok == RETURN) {
                        /* return jump - for HW2! */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else if (root->hdr.tok == BREAK) {
                        /* break jump - for HW2! */
                        // dany: might be used inside switch...case,then split into cases here.
                        return new LoopBreak();
                    } else if (root->hdr.tok == GOTO) {
                        /* GOTO jump - for HW2! */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    }
                    break;
                }

                case TN_SWITCH: {
                    /* Switch case - for HW2! */
                    TreeNode *switch_obj = new SwitchCond();
                    switch_obj->son1 = obj_tree(root->lnode);
                    switch_obj->son2 = obj_tree(root->rnode);
                    return switch_obj;
                }

                case TN_INDEX: {
                    /* call for array - for HW2! */
                    if (root->lnode->hdr.type == TN_IDENT) {  //array name
                        string name = reinterpret_cast<leafnode *>(root->lnode)->data.sval->str;
                        if (ST.is_array(name)) {
                            const Variable *var = ST.find(name);
                            string type = var->getType();
                            const OrderedDims &dims = var->getOrderedDims();
                            Index::setCurrentDims(const_cast<OrderedDims *>(&dims));
                            Index::setSizeOfBasic(ST.CalcSize(type));
                        }
                    } else if (root->lnode->hdr.type == TN_DEREF) { // just a pointer
                        Index::setCurrentDims(nullptr);
                        Index::setSizeOfBasic(1);
                    }
                    return new Index(root->lnode, root->rnode);
                }

                case TN_DEREF: {
                    /* pointer derefrence - for HW2! */
                    return new PtrDeref(root->rnode);
                }

                case TN_SELECT: {
                    /* struct case - for HW2! */
                    if (root->hdr.tok == ARROW) {
                        /* Struct select case "->" */
                        /* e.g. struct_variable->x; */
                        obj_tree(root->lnode);
                        obj_tree(root->rnode);
                    } else {
                        /* Struct select case "." */
                        /* e.g. struct_variable.x; */
                        node_type ln_t(root->lnode->hdr.which), rn_t(root->rnode->hdr.which);

                        string struct_field = reinterpret_cast<leafnode *>(root->rnode)->data.sval->str;
                        if (ln_t == LEAF_T && rn_t == LEAF_T) {
                            string struct_name = reinterpret_cast<leafnode *>(root->lnode)->data.sval->str;
                            return new StructDot(struct_name, struct_field, nullptr);
                        } else {
                            return new StructDot("", struct_field, root->lnode);
                        }

                    }
                    break;
                }

                case TN_ASSIGN: {
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
                }

                case TN_EXPR:
                    switch (root->hdr.tok) {
                        case CASE: {
                            /* you should not get here */
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;
                        }

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

                        default: {
                            obj_tree(root->lnode);
                            obj_tree(root->rnode);
                            break;
                        }
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

                case TN_LABEL: {
                    TreeNode *switch_label = new SwitchLabel();
                    switch_label->son1 = obj_tree(root->lnode);
                    switch_label->son2 = obj_tree(root->rnode);
                    return switch_label;
                }

                default: {
                    obj_tree(root->lnode);
                    obj_tree(root->rnode);
                }
            }
            break;

        case NONE_T: {
            printf("Error: Unknown node type!\n");
            exit(FAILURE);
        }
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

    SymbolTable::print();
}