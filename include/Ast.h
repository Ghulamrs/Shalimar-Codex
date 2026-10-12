#ifndef SHALIMAR_AST_H
#define SHALIMAR_AST_H

#include <string>
#include <vector>
#include <deque>
#include <stdexcept>
#include <typeinfo>
#include <cstddef>

namespace shalimar {

// Nodes live by value in typed container pools. Edges carry pool indices.
// The arena observer is non-owning; Program outlives every handle.
struct AstNode { virtual ~AstNode() {} };
class AstArenaAccess {
public:
    virtual ~AstArenaAccess() {}
    virtual AstNode& resolve(int pool, size_t index) = 0;
};
class AstArena;

// C++98 value-only typed node identifier. It owns no memory, contains no
// pointer, and remains valid when a deque pool grows. Resolve it explicitly
// through its owning arena; never cache references across arena destruction.
// Named C++98 value-only node identifiers, never owning node memory.
struct NodeIdBase {
    int pool_;
    size_t index_;
    size_t arenaId_;
    NodeIdBase() : pool_(-1), index_(0), arenaId_(0) {}
    NodeIdBase(int p, size_t i, size_t a) : pool_(p), index_(i), arenaId_(a) {
        if (p < 0 || p > 19 || a == 0) throw std::out_of_range("invalid AST node id");
    }
    int pool() const { return pool_; }
    size_t index() const { return index_; }
    bool valid() const { return pool_ >= 0 && arenaId_ != 0; }
};
struct LiteralExprId : NodeIdBase {
    LiteralExprId() {}
    LiteralExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct NameExprId : NodeIdBase {
    NameExprId() {}
    NameExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct IndexExprId : NodeIdBase {
    IndexExprId() {}
    IndexExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct AttributeExprId : NodeIdBase {
    AttributeExprId() {}
    AttributeExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct ArrayLiteralExprId : NodeIdBase {
    ArrayLiteralExprId() {}
    ArrayLiteralExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct UnaryExprId : NodeIdBase {
    UnaryExprId() {}
    UnaryExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct BinaryExprId : NodeIdBase {
    BinaryExprId() {}
    BinaryExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct CallExprId : NodeIdBase {
    CallExprId() {}
    CallExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct BlockStmtId : NodeIdBase {
    BlockStmtId() {}
    BlockStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct DeclStmtId : NodeIdBase {
    DeclStmtId() {}
    DeclStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct AssignStmtId : NodeIdBase {
    AssignStmtId() {}
    AssignStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct MultiAssignStmtId : NodeIdBase {
    MultiAssignStmtId() {}
    MultiAssignStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct ExprStmtId : NodeIdBase {
    ExprStmtId() {}
    ExprStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct ReturnStmtId : NodeIdBase {
    ReturnStmtId() {}
    ReturnStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct PrintStmtId : NodeIdBase {
    PrintStmtId() {}
    PrintStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct IfStmtId : NodeIdBase {
    IfStmtId() {}
    IfStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct WhileStmtId : NodeIdBase {
    WhileStmtId() {}
    WhileStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct ForStmtId : NodeIdBase {
    ForStmtId() {}
    ForStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct SimpleStmtId : NodeIdBase {
    SimpleStmtId() {}
    SimpleStmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct FunctionDefId : NodeIdBase {
    FunctionDefId() {}
    FunctionDefId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};

// Polymorphic AST edge IDs: value-only, arena-bound and non-template.
// These permit heterogeneous expression/statement child relationships.
struct ExprId : NodeIdBase {
    ExprId() {}
    ExprId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};
struct StmtId : NodeIdBase {
    StmtId() {}
    StmtId(int p, size_t i, size_t a) : NodeIdBase(p, i, a) {}
};

// Named, non-template value handle for function declarations.
// A valid identifier is bound to one arena and cannot be dereferenced alone.
struct FunctionId {
    size_t index;
    size_t arenaIdentity;
    FunctionId() : index(0), arenaIdentity(0) {}
    FunctionId(size_t i, size_t owner) : index(i), arenaIdentity(owner) {}
    bool valid() const { return arenaIdentity != 0; }
};

template <typename T> class AstRef {
    AstArenaAccess* arena_;
    int pool_;
    size_t index_;
public:
    AstRef(int nullValue = 0) : arena_(0), pool_(0), index_(0) {
        if (nullValue != 0) throw std::logic_error("invalid null AST handle");
    }
    AstRef(AstArenaAccess& arena, int pool, size_t index)
        : arena_(&arena), pool_(pool), index_(index) {}
    template <typename U> AstRef(const AstRef<U>& other)
        : arena_(other.arena()), pool_(other.pool()), index_(other.index()) {
        // This conversion also enforces const correctness at compilation.
        if (other) {
            T& compatible = dynamic_cast<T&>(other.value());
            (void)compatible;
        }
    }
    AstArenaAccess* arena() const { return arena_; }
    int pool() const { return pool_; }
    size_t index() const { return index_; }
    T& value() const {
        if (!arena_) throw std::logic_error("null AST handle access");
        return dynamic_cast<T&>(arena_->resolve(pool_, index_));
    }
    // Compatibility bridge: operator-> must return a pointer in C++.
    T* operator->() const { return &value(); }
    T& operator*() const { return value(); }
    operator bool() const { return arena_ != 0; }
};
template <typename T, typename U> AstRef<T> astCast(const AstRef<U>& value) {
    if (!value) return AstRef<T>();
    try {
        T& compatible = dynamic_cast<T&>(value.value());
        (void)compatible;
        return AstRef<T>(*value.arena(), value.pool(), value.index());
    } catch (const std::bad_cast&) {
        return AstRef<T>();
    }
}

struct Expr : AstNode {
    virtual ~Expr() {}
};

struct LiteralExpr : Expr {
    std::string value;
    bool stringValue;

    LiteralExpr(const std::string& text, bool isString = false)
        : value(text), stringValue(isString) {}
};

struct NameExpr : Expr {
    std::string name;
    mutable std::string constantValue;

    explicit NameExpr(const std::string& value) : name(value) {}
};

struct IndexExpr : Expr {
    AstRef<Expr> base;
    AstRef<Expr> index;

    IndexExpr(AstRef<Expr> baseExpr, AstRef<Expr> indexExpr)
        : base(baseExpr), index(indexExpr) {}

};

struct AttributeExpr : Expr {
    AstRef<Expr> base;
    std::string attribute;
    AstRef<Expr> axis;

    AttributeExpr(AstRef<Expr> baseExpr, const std::string& name, AstRef<Expr> dimension = 0)
        : base(baseExpr), attribute(name), axis(dimension) {}

};

struct ArrayLiteralExpr : Expr {
    std::vector<AstRef<Expr> > values;

};

struct UnaryExpr : Expr {
    std::string op;
    AstRef<Expr> value;

    UnaryExpr(const std::string& operation, AstRef<Expr> operand)
        : op(operation), value(operand) {}

};

struct BinaryExpr : Expr {
    std::string op;
    AstRef<Expr> left;
    AstRef<Expr> right;

    BinaryExpr(const std::string& operation, AstRef<Expr> lhs, AstRef<Expr> rhs)
        : op(operation), left(lhs), right(rhs) {}

};

struct CallExpr : Expr {
    std::string name;
    std::vector<AstRef<Expr> > args;

    explicit CallExpr(const std::string& value) : name(value) {}

};

struct Stmt : AstNode {
    int sourceLine;

    Stmt() : sourceLine(0) {}
    virtual ~Stmt() {}
};

struct BlockStmt : Stmt {
    std::vector<AstRef<Stmt> > statements;

};

struct DeclStmt : Stmt {
    bool implicit;
    bool inferredString;
    std::string type;
    std::string name;
    std::vector<AstRef<Expr> > dimensions;
    AstRef<Expr> init;

    DeclStmt(const std::string& valueType, const std::string& valueName, int line = 0)
        : implicit(false), inferredString(false), type(valueType), name(valueName), init(0) { sourceLine = line; }

};

struct AssignStmt : Stmt {
    AstRef<Expr> target;
    std::string op;
    AstRef<Expr> value;

    AssignStmt(AstRef<Expr> lhs, const std::string& operation, AstRef<Expr> rhs)
        : target(lhs), op(operation), value(rhs) {}

};

struct MultiAssignStmt : Stmt {
    std::vector<std::string> names;
    AstRef<CallExpr> call;

    MultiAssignStmt() : call(0) {}

};

struct ExprStmt : Stmt {
    AstRef<Expr> value;

    explicit ExprStmt(AstRef<Expr> expression) : value(expression) {}

};

struct ReturnStmt : Stmt {
    bool parenthesized;
    std::vector<AstRef<Expr> > values;

    ReturnStmt() : parenthesized(false) {}

};

struct PrintStmt : Stmt {
    bool newline;
    std::vector<AstRef<Expr> > items;

    explicit PrintStmt(bool appendNewline) : newline(appendNewline) {}

};

struct IfStmt : Stmt {
    AstRef<Expr> condition;
    AstRef<BlockStmt> yes;
    AstRef<BlockStmt> no;

    IfStmt(AstRef<Expr> test, AstRef<BlockStmt> thenBlock, AstRef<BlockStmt> elseBlock)
        : condition(test), yes(thenBlock), no(elseBlock) {}

};

struct WhileStmt : Stmt {
    AstRef<Expr> condition;
    AstRef<BlockStmt> body;

    WhileStmt(AstRef<Expr> test, AstRef<BlockStmt> loopBody)
        : condition(test), body(loopBody) {}

};

struct ForStmt : Stmt {
    mutable bool realCounter;
    std::string variable;
    AstRef<Expr> limit;
    AstRef<Expr> start;
    AstRef<Expr> step;
    AstRef<BlockStmt> body;

    ForStmt(const std::string& name, AstRef<Expr> end, AstRef<Expr> begin,
            AstRef<Expr> increment, AstRef<BlockStmt> loopBody)
        : realCounter(false), variable(name), limit(end), start(begin), step(increment), body(loopBody) {}

};

struct SimpleStmt : Stmt {
    std::string keyword;

    explicit SimpleStmt(const std::string& value) : keyword(value) {}
};

struct Parameter {
    std::string type;
    std::string name;
    bool byReference;
    int arrayRank;

    Parameter(const std::string& valueType, const std::string& valueName,
              bool reference = false, int rank = 0)
        : type(valueType), name(valueName), byReference(reference), arrayRank(rank) {}
};

struct FunctionDef : AstNode {
    std::string name;
    int sourceLine;
    std::vector<std::string> returnTypes;
    std::vector<Parameter> parameters;
    AstRef<BlockStmt> body;

    FunctionDef() : sourceLine(0), body(0) {}

};

struct LibraryUse {
    std::string name;
    int sourceLine;

    LibraryUse(const std::string& value, int line)
        : name(value), sourceLine(line) {}
};

class AstArena : public AstArenaAccess {
    std::deque<LiteralExpr> poolLiteralExpr;
    std::deque<NameExpr> poolNameExpr;
    std::deque<IndexExpr> poolIndexExpr;
    std::deque<AttributeExpr> poolAttributeExpr;
    std::deque<ArrayLiteralExpr> poolArrayLiteralExpr;
    std::deque<UnaryExpr> poolUnaryExpr;
    std::deque<BinaryExpr> poolBinaryExpr;
    std::deque<CallExpr> poolCallExpr;
    std::deque<BlockStmt> poolBlockStmt;
    std::deque<DeclStmt> poolDeclStmt;
    std::deque<AssignStmt> poolAssignStmt;
    std::deque<MultiAssignStmt> poolMultiAssignStmt;
    std::deque<ExprStmt> poolExprStmt;
    std::deque<ReturnStmt> poolReturnStmt;
    std::deque<PrintStmt> poolPrintStmt;
    std::deque<IfStmt> poolIfStmt;
    std::deque<WhileStmt> poolWhileStmt;
    std::deque<ForStmt> poolForStmt;
    std::deque<SimpleStmt> poolSimpleStmt;
    std::deque<FunctionDef> poolFunctionDef;
    size_t identity_;
    static size_t nextIdentity() {
        static size_t next = 0;
        if (++next == 0) throw std::overflow_error("AST arena identity exhausted");
        return next;
    }
    AstArena(const AstArena&);
    AstArena& operator=(const AstArena&);
public:
    AstArena() : identity_(nextIdentity()) {}
    size_t arenaIdentity() const { return identity_; }
    AstRef<LiteralExpr> add(const LiteralExpr& value) {
        poolLiteralExpr.push_back(value);
        return AstRef<LiteralExpr>(*this, 0, poolLiteralExpr.size() - 1);
    }
    AstRef<NameExpr> add(const NameExpr& value) {
        poolNameExpr.push_back(value);
        return AstRef<NameExpr>(*this, 1, poolNameExpr.size() - 1);
    }
    AstRef<IndexExpr> add(const IndexExpr& value) {
        poolIndexExpr.push_back(value);
        return AstRef<IndexExpr>(*this, 2, poolIndexExpr.size() - 1);
    }
    AstRef<AttributeExpr> add(const AttributeExpr& value) {
        poolAttributeExpr.push_back(value);
        return AstRef<AttributeExpr>(*this, 3, poolAttributeExpr.size() - 1);
    }
    AstRef<ArrayLiteralExpr> add(const ArrayLiteralExpr& value) {
        poolArrayLiteralExpr.push_back(value);
        return AstRef<ArrayLiteralExpr>(*this, 4, poolArrayLiteralExpr.size() - 1);
    }
    AstRef<UnaryExpr> add(const UnaryExpr& value) {
        poolUnaryExpr.push_back(value);
        return AstRef<UnaryExpr>(*this, 5, poolUnaryExpr.size() - 1);
    }
    AstRef<BinaryExpr> add(const BinaryExpr& value) {
        poolBinaryExpr.push_back(value);
        return AstRef<BinaryExpr>(*this, 6, poolBinaryExpr.size() - 1);
    }
    AstRef<CallExpr> add(const CallExpr& value) {
        poolCallExpr.push_back(value);
        return AstRef<CallExpr>(*this, 7, poolCallExpr.size() - 1);
    }
    AstRef<BlockStmt> add(const BlockStmt& value) {
        poolBlockStmt.push_back(value);
        return AstRef<BlockStmt>(*this, 8, poolBlockStmt.size() - 1);
    }
    AstRef<DeclStmt> add(const DeclStmt& value) {
        poolDeclStmt.push_back(value);
        return AstRef<DeclStmt>(*this, 9, poolDeclStmt.size() - 1);
    }
    AstRef<AssignStmt> add(const AssignStmt& value) {
        poolAssignStmt.push_back(value);
        return AstRef<AssignStmt>(*this, 10, poolAssignStmt.size() - 1);
    }
    AstRef<MultiAssignStmt> add(const MultiAssignStmt& value) {
        poolMultiAssignStmt.push_back(value);
        return AstRef<MultiAssignStmt>(*this, 11, poolMultiAssignStmt.size() - 1);
    }
    AstRef<ExprStmt> add(const ExprStmt& value) {
        poolExprStmt.push_back(value);
        return AstRef<ExprStmt>(*this, 12, poolExprStmt.size() - 1);
    }
    AstRef<ReturnStmt> add(const ReturnStmt& value) {
        poolReturnStmt.push_back(value);
        return AstRef<ReturnStmt>(*this, 13, poolReturnStmt.size() - 1);
    }
    AstRef<PrintStmt> add(const PrintStmt& value) {
        poolPrintStmt.push_back(value);
        return AstRef<PrintStmt>(*this, 14, poolPrintStmt.size() - 1);
    }
    AstRef<IfStmt> add(const IfStmt& value) {
        poolIfStmt.push_back(value);
        return AstRef<IfStmt>(*this, 15, poolIfStmt.size() - 1);
    }
    AstRef<WhileStmt> add(const WhileStmt& value) {
        poolWhileStmt.push_back(value);
        return AstRef<WhileStmt>(*this, 16, poolWhileStmt.size() - 1);
    }
    AstRef<ForStmt> add(const ForStmt& value) {
        poolForStmt.push_back(value);
        return AstRef<ForStmt>(*this, 17, poolForStmt.size() - 1);
    }
    AstRef<SimpleStmt> add(const SimpleStmt& value) {
        poolSimpleStmt.push_back(value);
        return AstRef<SimpleStmt>(*this, 18, poolSimpleStmt.size() - 1);
    }
    AstRef<FunctionDef> add(const FunctionDef& value) {
        poolFunctionDef.push_back(value);
        return AstRef<FunctionDef>(*this, 19, poolFunctionDef.size() - 1);
    }
    FunctionId functionId(const AstRef<FunctionDef>& handle) const {
        if (!handle) return FunctionId();
        if (handle.arena() != this || handle.pool() != 19)
            throw std::logic_error("function handle belongs to another arena or pool");
        return FunctionId(handle.index(), identity_);
    }
    FunctionDef& function(FunctionId identifier) {
        if (!identifier.valid() || identifier.arenaIdentity != identity_)
            throw std::logic_error("invalid or foreign function identifier");
        return poolFunctionDef.at(identifier.index);
    }
    const FunctionDef& function(FunctionId identifier) const {
        if (!identifier.valid() || identifier.arenaIdentity != identity_)
            throw std::logic_error("invalid or foreign function identifier");
        return poolFunctionDef.at(identifier.index);
    }
    // New construction API: callers receive value-only identifiers directly.
    // Checked, non-template access for polymorphic child IDs.
    Expr& node(const ExprId& id) {
        if (!id.valid() || id.arenaId_ != identity_ || id.pool_ > 7)
            throw std::logic_error("invalid or foreign expression ID");
        return dynamic_cast<Expr&>(resolve(id.pool_, id.index_));
    }
    const Expr& node(const ExprId& id) const {
        return const_cast<AstArena*>(this)->node(id);
    }
    Stmt& node(const StmtId& id) {
        if (!id.valid() || id.arenaId_ != identity_ || id.pool_ < 8 || id.pool_ > 18)
            throw std::logic_error("invalid or foreign statement ID");
        return dynamic_cast<Stmt&>(resolve(id.pool_, id.index_));
    }
    const Stmt& node(const StmtId& id) const {
        return const_cast<AstArena*>(this)->node(id);
    }
    // Checked non-template downcasts: mismatched node kinds fail closed.
    LiteralExprId asLiteralExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 0) throw std::logic_error("AST node kind mismatch: LiteralExpr");
        return LiteralExprId(value.pool_, value.index_, value.arenaId_);
    }
    NameExprId asNameExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 1) throw std::logic_error("AST node kind mismatch: NameExpr");
        return NameExprId(value.pool_, value.index_, value.arenaId_);
    }
    IndexExprId asIndexExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 2) throw std::logic_error("AST node kind mismatch: IndexExpr");
        return IndexExprId(value.pool_, value.index_, value.arenaId_);
    }
    AttributeExprId asAttributeExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 3) throw std::logic_error("AST node kind mismatch: AttributeExpr");
        return AttributeExprId(value.pool_, value.index_, value.arenaId_);
    }
    ArrayLiteralExprId asArrayLiteralExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 4) throw std::logic_error("AST node kind mismatch: ArrayLiteralExpr");
        return ArrayLiteralExprId(value.pool_, value.index_, value.arenaId_);
    }
    UnaryExprId asUnaryExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 5) throw std::logic_error("AST node kind mismatch: UnaryExpr");
        return UnaryExprId(value.pool_, value.index_, value.arenaId_);
    }
    BinaryExprId asBinaryExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 6) throw std::logic_error("AST node kind mismatch: BinaryExpr");
        return BinaryExprId(value.pool_, value.index_, value.arenaId_);
    }
    CallExprId asCallExpr(const ExprId& value) const {
        node(value);
        if (value.pool_ != 7) throw std::logic_error("AST node kind mismatch: CallExpr");
        return CallExprId(value.pool_, value.index_, value.arenaId_);
    }
    BlockStmtId asBlockStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 8) throw std::logic_error("AST node kind mismatch: BlockStmt");
        return BlockStmtId(value.pool_, value.index_, value.arenaId_);
    }
    DeclStmtId asDeclStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 9) throw std::logic_error("AST node kind mismatch: DeclStmt");
        return DeclStmtId(value.pool_, value.index_, value.arenaId_);
    }
    AssignStmtId asAssignStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 10) throw std::logic_error("AST node kind mismatch: AssignStmt");
        return AssignStmtId(value.pool_, value.index_, value.arenaId_);
    }
    MultiAssignStmtId asMultiAssignStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 11) throw std::logic_error("AST node kind mismatch: MultiAssignStmt");
        return MultiAssignStmtId(value.pool_, value.index_, value.arenaId_);
    }
    ExprStmtId asExprStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 12) throw std::logic_error("AST node kind mismatch: ExprStmt");
        return ExprStmtId(value.pool_, value.index_, value.arenaId_);
    }
    ReturnStmtId asReturnStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 13) throw std::logic_error("AST node kind mismatch: ReturnStmt");
        return ReturnStmtId(value.pool_, value.index_, value.arenaId_);
    }
    PrintStmtId asPrintStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 14) throw std::logic_error("AST node kind mismatch: PrintStmt");
        return PrintStmtId(value.pool_, value.index_, value.arenaId_);
    }
    IfStmtId asIfStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 15) throw std::logic_error("AST node kind mismatch: IfStmt");
        return IfStmtId(value.pool_, value.index_, value.arenaId_);
    }
    WhileStmtId asWhileStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 16) throw std::logic_error("AST node kind mismatch: WhileStmt");
        return WhileStmtId(value.pool_, value.index_, value.arenaId_);
    }
    ForStmtId asForStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 17) throw std::logic_error("AST node kind mismatch: ForStmt");
        return ForStmtId(value.pool_, value.index_, value.arenaId_);
    }
    SimpleStmtId asSimpleStmt(const StmtId& value) const {
        node(value);
        if (value.pool_ != 18) throw std::logic_error("AST node kind mismatch: SimpleStmt");
        return SimpleStmtId(value.pool_, value.index_, value.arenaId_);
    }
    ExprId edge(const LiteralExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const NameExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const IndexExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const AttributeExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const ArrayLiteralExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const UnaryExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const BinaryExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    ExprId edge(const CallExprId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign expression edge");
        return ExprId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const BlockStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const DeclStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const AssignStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const MultiAssignStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const ExprStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const ReturnStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const PrintStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const IfStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const WhileStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const ForStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    StmtId edge(const SimpleStmtId& id) const {
        if (!id.valid() || id.arenaId_ != identity_) throw std::logic_error("foreign statement edge");
        return StmtId(id.pool_, id.index_, id.arenaId_);
    }
    LiteralExprId addId(const LiteralExpr& value) { return id(add(value)); }
    NameExprId addId(const NameExpr& value) { return id(add(value)); }
    IndexExprId addId(const IndexExpr& value) { return id(add(value)); }
    AttributeExprId addId(const AttributeExpr& value) { return id(add(value)); }
    ArrayLiteralExprId addId(const ArrayLiteralExpr& value) { return id(add(value)); }
    UnaryExprId addId(const UnaryExpr& value) { return id(add(value)); }
    BinaryExprId addId(const BinaryExpr& value) { return id(add(value)); }
    CallExprId addId(const CallExpr& value) { return id(add(value)); }
    // Checked compatibility bridges until AST child edges are converted to IDs.
    AstRef<LiteralExpr> legacyLiteralRef(const LiteralExprId& handle) {
        LiteralExpr& checked = node(handle);
        (void)checked;
        return AstRef<LiteralExpr>(*this, handle.pool_, handle.index_);
    }
    AstRef<NameExpr> legacyNameRef(const NameExprId& handle) {
        NameExpr& checked = node(handle);
        (void)checked;
        return AstRef<NameExpr>(*this, handle.pool_, handle.index_);
    }
    // Temporary, checked compatibility bridge while AST child fields use AstRef.
    AstRef<CallExpr> legacyCallRef(const CallExprId& handle) {
        CallExpr& checked = node(handle);
        (void)checked;
        return AstRef<CallExpr>(*this, handle.pool_, handle.index_);
    }
    AstRef<UnaryExpr> legacyUnaryRef(const UnaryExprId& handle) {
        UnaryExpr& checked = node(handle);
        (void)checked;
        return AstRef<UnaryExpr>(*this, handle.pool_, handle.index_);
    }
    AstRef<IndexExpr> legacyIndexRef(const IndexExprId& handle) {
        IndexExpr& checked = node(handle);
        (void)checked;
        return AstRef<IndexExpr>(*this, handle.pool_, handle.index_);
    }
    AstRef<AttributeExpr> legacyAttributeRef(const AttributeExprId& handle) {
        AttributeExpr& checked = node(handle);
        (void)checked;
        return AstRef<AttributeExpr>(*this, handle.pool_, handle.index_);
    }
    BlockStmtId addId(const BlockStmt& value) { return id(add(value)); }
    DeclStmtId addId(const DeclStmt& value) { return id(add(value)); }
    AssignStmtId addId(const AssignStmt& value) { return id(add(value)); }
    MultiAssignStmtId addId(const MultiAssignStmt& value) { return id(add(value)); }
    ExprStmtId addId(const ExprStmt& value) { return id(add(value)); }
    ReturnStmtId addId(const ReturnStmt& value) { return id(add(value)); }
    PrintStmtId addId(const PrintStmt& value) { return id(add(value)); }
    IfStmtId addId(const IfStmt& value) { return id(add(value)); }
    WhileStmtId addId(const WhileStmt& value) { return id(add(value)); }
    ForStmtId addId(const ForStmt& value) { return id(add(value)); }
    SimpleStmtId addId(const SimpleStmt& value) { return id(add(value)); }
    FunctionDefId addId(const FunctionDef& value) { return id(add(value)); }
    // Checked, arena-bound ID construction and node access.
    LiteralExprId id(const AstRef<LiteralExpr>& h) const {
        if (!h) return LiteralExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return LiteralExprId(h.pool(), h.index(), identity_);
    }
    LiteralExpr& node(LiteralExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<LiteralExpr&>(resolve(h.pool(), h.index()));
    }
    const LiteralExpr& node(LiteralExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const LiteralExpr&>(resolve(h.pool(), h.index()));
    }
    NameExprId id(const AstRef<NameExpr>& h) const {
        if (!h) return NameExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return NameExprId(h.pool(), h.index(), identity_);
    }
    NameExpr& node(NameExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<NameExpr&>(resolve(h.pool(), h.index()));
    }
    const NameExpr& node(NameExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const NameExpr&>(resolve(h.pool(), h.index()));
    }
    IndexExprId id(const AstRef<IndexExpr>& h) const {
        if (!h) return IndexExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return IndexExprId(h.pool(), h.index(), identity_);
    }
    IndexExpr& node(IndexExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<IndexExpr&>(resolve(h.pool(), h.index()));
    }
    const IndexExpr& node(IndexExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const IndexExpr&>(resolve(h.pool(), h.index()));
    }
    AttributeExprId id(const AstRef<AttributeExpr>& h) const {
        if (!h) return AttributeExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return AttributeExprId(h.pool(), h.index(), identity_);
    }
    AttributeExpr& node(AttributeExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<AttributeExpr&>(resolve(h.pool(), h.index()));
    }
    const AttributeExpr& node(AttributeExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const AttributeExpr&>(resolve(h.pool(), h.index()));
    }
    ArrayLiteralExprId id(const AstRef<ArrayLiteralExpr>& h) const {
        if (!h) return ArrayLiteralExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return ArrayLiteralExprId(h.pool(), h.index(), identity_);
    }
    ArrayLiteralExpr& node(ArrayLiteralExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<ArrayLiteralExpr&>(resolve(h.pool(), h.index()));
    }
    const ArrayLiteralExpr& node(ArrayLiteralExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const ArrayLiteralExpr&>(resolve(h.pool(), h.index()));
    }
    UnaryExprId id(const AstRef<UnaryExpr>& h) const {
        if (!h) return UnaryExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return UnaryExprId(h.pool(), h.index(), identity_);
    }
    UnaryExpr& node(UnaryExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<UnaryExpr&>(resolve(h.pool(), h.index()));
    }
    const UnaryExpr& node(UnaryExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const UnaryExpr&>(resolve(h.pool(), h.index()));
    }
    BinaryExprId id(const AstRef<BinaryExpr>& h) const {
        if (!h) return BinaryExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return BinaryExprId(h.pool(), h.index(), identity_);
    }
    BinaryExpr& node(BinaryExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<BinaryExpr&>(resolve(h.pool(), h.index()));
    }
    const BinaryExpr& node(BinaryExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const BinaryExpr&>(resolve(h.pool(), h.index()));
    }
    CallExprId id(const AstRef<CallExpr>& h) const {
        if (!h) return CallExprId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return CallExprId(h.pool(), h.index(), identity_);
    }
    CallExpr& node(CallExprId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<CallExpr&>(resolve(h.pool(), h.index()));
    }
    const CallExpr& node(CallExprId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const CallExpr&>(resolve(h.pool(), h.index()));
    }
    BlockStmtId id(const AstRef<BlockStmt>& h) const {
        if (!h) return BlockStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return BlockStmtId(h.pool(), h.index(), identity_);
    }
    BlockStmt& node(BlockStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<BlockStmt&>(resolve(h.pool(), h.index()));
    }
    const BlockStmt& node(BlockStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const BlockStmt&>(resolve(h.pool(), h.index()));
    }
    DeclStmtId id(const AstRef<DeclStmt>& h) const {
        if (!h) return DeclStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return DeclStmtId(h.pool(), h.index(), identity_);
    }
    DeclStmt& node(DeclStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<DeclStmt&>(resolve(h.pool(), h.index()));
    }
    const DeclStmt& node(DeclStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const DeclStmt&>(resolve(h.pool(), h.index()));
    }
    AssignStmtId id(const AstRef<AssignStmt>& h) const {
        if (!h) return AssignStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return AssignStmtId(h.pool(), h.index(), identity_);
    }
    AssignStmt& node(AssignStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<AssignStmt&>(resolve(h.pool(), h.index()));
    }
    const AssignStmt& node(AssignStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const AssignStmt&>(resolve(h.pool(), h.index()));
    }
    MultiAssignStmtId id(const AstRef<MultiAssignStmt>& h) const {
        if (!h) return MultiAssignStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return MultiAssignStmtId(h.pool(), h.index(), identity_);
    }
    MultiAssignStmt& node(MultiAssignStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<MultiAssignStmt&>(resolve(h.pool(), h.index()));
    }
    const MultiAssignStmt& node(MultiAssignStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const MultiAssignStmt&>(resolve(h.pool(), h.index()));
    }
    ExprStmtId id(const AstRef<ExprStmt>& h) const {
        if (!h) return ExprStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return ExprStmtId(h.pool(), h.index(), identity_);
    }
    ExprStmt& node(ExprStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<ExprStmt&>(resolve(h.pool(), h.index()));
    }
    const ExprStmt& node(ExprStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const ExprStmt&>(resolve(h.pool(), h.index()));
    }
    ReturnStmtId id(const AstRef<ReturnStmt>& h) const {
        if (!h) return ReturnStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return ReturnStmtId(h.pool(), h.index(), identity_);
    }
    ReturnStmt& node(ReturnStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<ReturnStmt&>(resolve(h.pool(), h.index()));
    }
    const ReturnStmt& node(ReturnStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const ReturnStmt&>(resolve(h.pool(), h.index()));
    }
    PrintStmtId id(const AstRef<PrintStmt>& h) const {
        if (!h) return PrintStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return PrintStmtId(h.pool(), h.index(), identity_);
    }
    PrintStmt& node(PrintStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<PrintStmt&>(resolve(h.pool(), h.index()));
    }
    const PrintStmt& node(PrintStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const PrintStmt&>(resolve(h.pool(), h.index()));
    }
    IfStmtId id(const AstRef<IfStmt>& h) const {
        if (!h) return IfStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return IfStmtId(h.pool(), h.index(), identity_);
    }
    IfStmt& node(IfStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<IfStmt&>(resolve(h.pool(), h.index()));
    }
    const IfStmt& node(IfStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const IfStmt&>(resolve(h.pool(), h.index()));
    }
    WhileStmtId id(const AstRef<WhileStmt>& h) const {
        if (!h) return WhileStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return WhileStmtId(h.pool(), h.index(), identity_);
    }
    WhileStmt& node(WhileStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<WhileStmt&>(resolve(h.pool(), h.index()));
    }
    const WhileStmt& node(WhileStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const WhileStmt&>(resolve(h.pool(), h.index()));
    }
    ForStmtId id(const AstRef<ForStmt>& h) const {
        if (!h) return ForStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return ForStmtId(h.pool(), h.index(), identity_);
    }
    ForStmt& node(ForStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<ForStmt&>(resolve(h.pool(), h.index()));
    }
    const ForStmt& node(ForStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const ForStmt&>(resolve(h.pool(), h.index()));
    }
    SimpleStmtId id(const AstRef<SimpleStmt>& h) const {
        if (!h) return SimpleStmtId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return SimpleStmtId(h.pool(), h.index(), identity_);
    }
    SimpleStmt& node(SimpleStmtId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<SimpleStmt&>(resolve(h.pool(), h.index()));
    }
    const SimpleStmt& node(SimpleStmtId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const SimpleStmt&>(resolve(h.pool(), h.index()));
    }
    FunctionDefId id(const AstRef<FunctionDef>& h) const {
        if (!h) return FunctionDefId();
        if (h.arena() != this) throw std::logic_error("foreign AST handle");
        return FunctionDefId(h.pool(), h.index(), identity_);
    }
    FunctionDef& node(FunctionDefId h) {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<FunctionDef&>(resolve(h.pool(), h.index()));
    }
    const FunctionDef& node(FunctionDefId h) const {
        if (!h.valid() || h.arenaId_ != identity_) throw std::logic_error("invalid or foreign AST ID");
        return dynamic_cast<const FunctionDef&>(resolve(h.pool(), h.index()));
    }
    const AstNode& resolve(int pool, size_t index) const {
        return const_cast<AstArena&>(*this).resolve(pool, index);
    }
    AstNode& resolve(int pool, size_t index) {
        switch (pool) {
        case 0: return poolLiteralExpr.at(index);
        case 1: return poolNameExpr.at(index);
        case 2: return poolIndexExpr.at(index);
        case 3: return poolAttributeExpr.at(index);
        case 4: return poolArrayLiteralExpr.at(index);
        case 5: return poolUnaryExpr.at(index);
        case 6: return poolBinaryExpr.at(index);
        case 7: return poolCallExpr.at(index);
        case 8: return poolBlockStmt.at(index);
        case 9: return poolDeclStmt.at(index);
        case 10: return poolAssignStmt.at(index);
        case 11: return poolMultiAssignStmt.at(index);
        case 12: return poolExprStmt.at(index);
        case 13: return poolReturnStmt.at(index);
        case 14: return poolPrintStmt.at(index);
        case 15: return poolIfStmt.at(index);
        case 16: return poolWhileStmt.at(index);
        case 17: return poolForStmt.at(index);
        case 18: return poolSimpleStmt.at(index);
        case 19: return poolFunctionDef.at(index);
        default: throw std::logic_error("invalid AST pool");
        }
    }
};

struct Program {
    mutable AstArena arena;
private:
    Program(const Program&);
    Program& operator=(const Program&);
public:
    Program() {}
    std::vector<LibraryUse> libraryUses;
    std::vector<AstRef<FunctionDef> > functions;
    std::vector<AstRef<DeclStmt> > globals;

    FunctionId findNamedFunction(const std::string& name) const {
        for (size_t i = 0; i < functions.size(); ++i) {
            const FunctionId identifier = arena.functionId(functions[i]);
            if (arena.function(identifier).name == name) return identifier;
        }
        return FunctionId();
    }

    FunctionDefId findFunctionId(const std::string& name) const {
        for (size_t i = 0; i < functions.size(); ++i) {
            FunctionDefId identifier = arena.id(functions[i]);
            if (arena.node(identifier).name == name) return identifier;
        }
        return FunctionDefId();
    }

    AstRef<FunctionDef> findFunction(const std::string& name) const {
        for (size_t i = 0; i < functions.size(); ++i) {
            if (functions[i]->name == name) {
                return functions[i];
            }
        }
        return 0;
    }
};

} // namespace shalimar

#endif // SHALIMAR_AST_H
